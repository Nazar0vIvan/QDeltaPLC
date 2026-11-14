#include "devices/plc/plcdevice.h"
#include "common/socketconfigutils.h"

#include <winsock2.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTcpSocket>
#include <QTimer>

namespace {

// One request is in flight at a time; its reply must arrive within this time.
constexpr int ReplyTimeoutMs = 1000;
// SNAPSHOT period while idle: keeps the PLC socket below its 10 s Keep Alive Timer.
constexpr int HeartbeatMs = 2000;

// KUKA Automatic External outputs wired to PLC module 1 inputs X1.n.
constexpr int X1AutExt = 0;
constexpr int X1ProAct = 4;
constexpr int X1PgnoReq = 5;
constexpr int X1ApplRun = 6;

// records/plc_io_*.csv: one row per change, with the full current state of the robot interface.
const QString LogHeader = QStringLiteral(
    "time;event;X1.0 AUT_EXT;X1.2 PERI_RDY;X1.3 STOPMESS;X1.4 PRO_ACT;X1.5 PGNO_REQ;X1.6 APPL_RUN;"
    "X1.7 IN_HOME;Y1.0 EXT_START;Y1.1 DRIVES_ON;Y1.2 CONF_MESS;Y1.3 PGNO0;Y1.4 PGNO1;step;program;result\n");
const QStringList StepNames{"IDLE", "PREP", "START", "SELECT"};
const QStringList ResultNames{"-", "STARTED", "RESUMED", "TIMEOUT", "ABORTED", "AUT_EXT_LOST"};

bool matchesResponseField(const QVariantMap& data, const char* key, uint expected)
{
  return data.contains(key) && data.value(key).toUInt() == expected;
}

// PLC E_RSTEP / E_RRESULT code as its name; an unknown code as its number.
QString codeName(const QVariant& code, const QStringList& names)
{
  const int value = code.toInt();
  return value >= 0 && value < names.size() ? names.at(value) : QString::number(value);
}

QString bitText(const QVariantList& bits, int index)
{
  return bits.value(index).toBool() ? QStringLiteral("1") : QStringLiteral("0");
}

} // namespace

PlcDevice::PlcDevice(const QString& name, QObject* parent) : AbstractDevice(name, parent) {}

void PlcDevice::startDevice()
{
  Q_ASSERT(!m_sock);
  Q_ASSERT(!m_mgr);

  m_sock = new QTcpSocket(this);
  m_mgr = new PlcMessageManager(this);

  m_replyTimer = new QTimer(this);
  m_replyTimer->setSingleShot(true);
  m_replyTimer->setInterval(ReplyTimeoutMs);
  m_heartbeat = new QTimer(this);
  m_heartbeat->setInterval(HeartbeatMs);
  m_log = new QFile(this);

  attachSocket(m_sock);

  QObject::connect(m_sock, &QTcpSocket::connected, this, &PlcDevice::onConnected);
  QObject::connect(m_sock, &QTcpSocket::readyRead, this, &PlcDevice::onReadyRead);
  QObject::connect(m_sock, &QTcpSocket::stateChanged, this, &PlcDevice::onStateChanged);
  QObject::connect(m_replyTimer, &QTimer::timeout, this, &PlcDevice::onReplyTimeout);
  QObject::connect(m_heartbeat, &QTimer::timeout, this, &PlcDevice::onHeartbeat);
}

void PlcDevice::stopDevice()
{
  if (!m_sock) return;

  disconnect();

  delete m_sock;
  m_sock = nullptr;

  delete m_mgr;
  m_mgr = nullptr;

  delete m_replyTimer;
  m_replyTimer = nullptr;
  delete m_heartbeat;
  m_heartbeat = nullptr;
  delete m_log;
  m_log = nullptr;

  m_rx.clear();
  m_pend.clear();
}

void PlcDevice::connect(const QVariantMap& config)
{
  if (!m_sock) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << "PLC device is not started";
    return;
  }

  if (!config.isEmpty()) {
    const QHostAddress localAddr(config.value("localAddress").toString());
    const QHostAddress addr(config.value("peerAddress").toString());

    const auto localPort = parseSocketPort(config.value("localPort"));
    const auto port = parseSocketPort(config.value("peerPort"));

    if (localAddr.isNull() || !localPort || addr.isNull() || !port) {
      qInfo().noquote() << "[ERROR]" << objectName() + ":" << "Invalid socket configuration";
      return;
    }

    m_la = localAddr;
    m_lp = *localPort;
    m_pa = addr;
    m_pp = *port;
  }

  if (m_la.isNull() || m_lp == 0 || m_pa.isNull() || m_pp == 0) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << "Socket configuration is incomplete";
    return;
  }

  if (m_sock->state() != QAbstractSocket::UnconnectedState) {
    m_sock->abort();
  }

  emit stateReady({{"connectionConfig", QVariantMap{
    {"localAddress", m_la.toString()}, {"localPort", m_lp},
    {"peerAddress", m_pa.toString()}, {"peerPort", m_pp}
  }}});

  // The PLC program expects this PC endpoint, including its source port.
  if (!m_sock->bind(m_la, m_lp)) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << QString("Bind failed: %1").arg(m_sock->errorString());
    m_sock->abort();
    return;
  }

  m_sock->connectToHost(m_pa, m_pp, QIODevice::ReadWrite);
}

void PlcDevice::disconnect()
{
  m_rx.clear();
  m_pend.clear();
  if (!m_sock) return;

  // A FIN close keeps the fixed-port connection in TIME_WAIT and the next connect fails with
  // WSAEADDRINUSE; SO_LINGER {1, 0} makes closesocket send RST, which leaves no TIME_WAIT.
  if (const qintptr fd = m_sock->socketDescriptor(); fd != -1) {
    const linger reset{1, 0};
    if (::setsockopt(static_cast<SOCKET>(fd), SOL_SOCKET, SO_LINGER,
                     reinterpret_cast<const char*>(&reset), sizeof(reset)) != 0) {
      qInfo().noquote() << "[ERROR]" << objectName() + ":"
          << QString("SO_LINGER failed: WSA error %1").arg(::WSAGetLastError());
    }
  }

  m_sock->abort();
}

void PlcDevice::setOutput(const QVariantMap& args)
{
  if (!m_sock || !m_mgr || m_sock->state() != QAbstractSocket::ConnectedState) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << "PLC is not connected";
    return;
  }

  const PlcMessageManager::ParseResult request = m_mgr->outputRequest(args);
  if (!request.ok()) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << "WRITE ERROR: " + QString::number(request.error);
    return;
  }

  writeMessage(request.data.toMap());
}

void PlcDevice::writeMessage(const QVariantMap& msg)
{
  // A request made without a connection is dropped, never sent at a later connect.
  if (!m_sock || m_sock->state() != QAbstractSocket::ConnectedState) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << "PLC is not connected";
    return;
  }

  m_queue.enqueue(msg);
  sendNext();
}

void PlcDevice::runRobotProgram(const QVariantMap& args)
{
  const PlcMessageManager::ParseResult request = m_mgr->robotProgramRequest(args);
  if (!request.ok()) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":" << "WRITE ERROR: " + QString::number(request.error);
    return;
  }

  writeLog(QString("run %1").arg(request.data.toMap().value("attr").toUInt()));
  writeMessage(request.data.toMap());
}

void PlcDevice::resumeRobot()
{
  writeLog(QStringLiteral("resume"));
  writeMessage({
    {"cmd", PlcMessageManager::SET_VAR}, {"var", PlcMessageManager::ROBOT_RESUME}, {"attr", 0}
  });
}

void PlcDevice::abortRobotStart()
{
  writeLog(QStringLiteral("abort"));
  writeMessage({
    {"cmd", PlcMessageManager::SET_VAR}, {"var", PlcMessageManager::ROBOT_ABORT}, {"attr", 0}
  });
}

void PlcDevice::sendNext()
{
  // The PLC's receive buffer holds one frame, so the next request waits for the pending reply.
  while (m_pend.isEmpty() && !m_queue.isEmpty()) {
    const QVariantMap msg = m_queue.dequeue();
    const auto tid = static_cast<quint8>(m_nextTid + 1);
    const PlcMessageManager::ParseResult built = m_mgr->buildReq(msg, tid);

    if (!built.ok()) {
      qInfo().noquote() << "[ERROR]" << objectName() + ":" << "WRITE ERROR: " + QString::number(built.error);
      continue;
    }

    m_nextTid = tid;
    const QByteArray data = built.data.toByteArray();
    const qint64 count = m_sock->write(swapBytes(data));
    if (count != data.size()) {
      qInfo().noquote() << "[ERROR]" << objectName() + ":"
          << QString("Incomplete PLC write: %1 of %2 bytes queued").arg(count).arg(data.size());
      // A partially queued frame cannot safely be followed by another request; abort clears the queue.
      if (count > 0) m_sock->abort();
      continue;
    }

    // SNAPSHOT is the 2 s heartbeat: logging it would flood the console.
    if (msg.value("cmd").toUInt() != PlcMessageManager::SNAPSHOT) {
      qInfo().noquote() << "[WRITE]" << objectName() + ":"
          << QString("TX: %1 (%2 bytes)").arg(QString(data.toHex(' ').toUpper())).arg(count);
    }
    m_pend.insert(tid, msg);
    m_replyTimer->start();
  }
}

void PlcDevice::onReplyTimeout()
{
  qInfo().noquote() << "[ERROR]" << objectName() + ":"
      << QString("PLC reply timeout for transaction %1").arg(static_cast<uint>(m_nextTid));
  writeLog(QString("reply timeout %1").arg(static_cast<uint>(m_nextTid)));
  m_pend.clear();
  sendNext();
}

void PlcDevice::onHeartbeat()
{
  if (m_queue.isEmpty() && m_pend.isEmpty()) writeMessage({{"cmd", PlcMessageManager::SNAPSHOT}});
}

void PlcDevice::onConnected()
{
  qInfo().noquote() << "[OK]" << objectName() + ":" << "Connection has been successfully established";

  // No request right at connect: the PLC never answered it (a timeout at every connect, 2026-11-13).
  // Its IO change frame on a new connection gives the state; the heartbeat follows in 2 s.
  m_heartbeat->start();
  openLog();
}

void PlcDevice::openLog()
{
  const QDir records(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("records")));
  const QString fileName = QStringLiteral("plc_io_%1.csv")
      .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));

  m_log->setFileName(records.filePath(fileName));
  if (!QDir().mkpath(records.absolutePath()) || !m_log->open(QIODevice::WriteOnly)) {
    qInfo().noquote() << "[ERROR]" << objectName() + ":"
        << QString("Cannot write the PLC log '%1'").arg(m_log->fileName());
    return;
  }

  m_log->write(LogHeader.toUtf8());
  m_log->flush();
}

void PlcDevice::writeLog(const QString& event)
{
  if (!m_log->isOpen()) return;

  const QVariantList x1 = m_logged.value("x1").toList();
  const QVariantList y1 = m_logged.value("y1").toList();
  QStringList cells{QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")), event};
  for (const int bit : {0, 2, 3, 4, 5, 6, 7}) cells << bitText(x1, bit);
  for (int bit = 0; bit < 5; ++bit) cells << bitText(y1, bit);
  cells << codeName(m_logged.value("robotStep"), StepNames)
        << m_logged.value("robotProgram").toString()
        << codeName(m_logged.value("robotResult"), ResultNames);

  m_log->write((cells.join(';') + '\n').toUtf8());
  m_log->flush();
}

bool PlcDevice::matchResponse(const QVariantMap& data)
{
  const auto tid = static_cast<quint8>(data.value("tid").toUInt());
  const auto pending = m_pend.constFind(tid);
  if (pending == m_pend.cend()) return false;
  const QVariantMap& request = *pending;
  const uint cmd = request.value("cmd").toUInt();
  if (data.value("cmd").toUInt() != cmd) return false;

  // Error replies contain only command/error/code, so no endpoint fields can be matched.
  if (data.value("type").toUInt() == PlcMessageManager::RESP_OK) {
    switch (cmd) {
      case PlcMessageManager::READ_IO:
      case PlcMessageManager::WRITE_IO:
        if (!matchesResponseField(data, "dev", cmd == PlcMessageManager::WRITE_IO
                             ? uint(PlcMessageManager::Y) : request.value("dev").toUInt())
            || !matchesResponseField(data, "module", request.value("module").toUInt())) return false;
        break;
      case PlcMessageManager::READ_REG:
      case PlcMessageManager::WRITE_REG:
        if (!matchesResponseField(data, "dev", PlcMessageManager::D)
            || !matchesResponseField(data, "addr", request.value("addr").toUInt())) return false;
        if (cmd == PlcMessageManager::WRITE_REG
            && !matchesResponseField(data, "value", request.value("value").toUInt())) return false;
        break;
      case PlcMessageManager::SET_VAR:
        if (!matchesResponseField(data, "var", request.value("var").toUInt())
            || !matchesResponseField(data, "attr", request.value("attr").toUInt())) return false;
        break;
      default: break;
    }
  }
  m_pend.remove(tid);
  m_replyTimer->stop();
  return true;
}

void PlcDevice::onReadyRead()
{
  processIncoming(m_sock->readAll());
}

void PlcDevice::processIncoming(const QByteArray& bytes)
{
  m_rx.append(bytes);

  while (m_rx.size() >= PlcMessageManager::RESP_SIZE) {
    const QByteArray frame = swapBytes(m_rx.left(PlcMessageManager::RESP_SIZE));

    m_rx.remove(0, PlcMessageManager::RESP_SIZE);

    const PlcMessageManager::ParseResult parsed = m_mgr->parseMessage(frame);

    if (!parsed.ok()) {
      qInfo().noquote() << "[ERROR]" << objectName() + ":" << "READ ERROR: " + QString::number(parsed.error);
      writeLog(QString("read error %1 frame %2")
                   .arg(static_cast<int>(parsed.error)).arg(QString(frame.toHex(' ').toUpper())));
      continue;
    }

    const QVariantMap data = parsed.data.toMap();

    if (data.contains("tid")) {
      if (!matchResponse(data)) {
        qInfo().noquote() << "[ERROR]" << objectName() + ":" << "Unmatched PLC response for transaction " + data.value("tid").toString();
        writeLog("unmatched reply " + data.value("tid").toString());
        continue;
      }
    }

    if (data.value("type").toUInt() == PlcMessageManager::RESP_ERR) {
      qInfo().noquote() << "[ERROR]" << objectName() + ":"
          << QString("PLC response error: cmd=%1, err=%2, code=%3")
                 .arg(data.value("cmd").toUInt())
                 .arg(data.value("err").toUInt())
                 .arg(data.value("code").toUInt());
      writeLog(QString("error cmd=%1 code=%2").arg(data.value("cmd").toUInt()).arg(data.value("code").toUInt()));

      continue;
    }

    publishData(data);
  }

  sendNext();
}

void PlcDevice::onStateChanged(QAbstractSocket::SocketState state)
{
  if (state != QAbstractSocket::UnconnectedState) return;

  m_rx.clear();
  m_queue.clear();
  m_pend.clear();
  m_replyTimer->stop();
  m_heartbeat->stop();
  m_programState = ProgramState::Unknown;
  m_resumeBlocked = false;
  m_log->close();
  m_logged.clear();

  // Clear the persistent PLC state seen by QML.
  emit stateReady({
    {"x1", QVariantList(8, false)},
    {"y1", QVariantList(8, false)},
    {"x2", QVariantList(8, false)},
    {"y2", QVariantList(8, false)},
    {"robotStep", 0},
    {"robotProgram", 0},
    {"robotResult", 0},
    {"robotProgramState", static_cast<int>(ProgramState::Unknown)},
    {"robotResumeBlocked", false}
  });
}

void PlcDevice::publishData(const QVariantMap& data)
{
  const quint8 type = static_cast<quint8>(data.value("type").toUInt());

  if (type == PlcMessageManager::RESP_OK) {
    const quint8 cmd = static_cast<quint8>(data.value("cmd").toUInt());

    if (cmd == PlcMessageManager::SNAPSHOT) {
      publishIo(data);
      return;
    }

    if (cmd == PlcMessageManager::WRITE_IO) {
      const int mod = data.value("module").toInt();

      if (mod == 1) {
        emit stateReady({{"y1", data.value("state")}});
      } else if (mod == 2) {
        emit stateReady({{"y2", data.value("state")}
        });
      }
      return;
    }
    return;
  }

  if (type == PlcMessageManager::CHG) {
    const quint8 chg = static_cast<quint8>(data.value("chg").toUInt());
    if (chg == PlcMessageManager::IOs) publishIo(data);
  }
}

void PlcDevice::publishIo(const QVariantMap& data)
{
  // The IO change frame and the SNAPSHOT reply carry the same IO bytes and start-sequence state.
  QVariantHash values;
  for (const char* key : {"x1", "y1", "x2", "y2", "robotStep", "robotProgram", "robotResult"})
    values.insert(key, data.value(key));

  // The heartbeat SNAPSHOT repeats unchanged values every 2 s; the log keeps only changes.
  bool changed = false;
  for (const char* key : {"x1", "y1", "robotStep", "robotProgram", "robotResult"}) {
    if (values.value(key) == m_logged.value(key)) continue;
    m_logged.insert(key, values.value(key));
    changed = true;
  }
  if (changed) writeLog(QStringLiteral("io"));

  const QVariantList x1 = data.value("x1").toList();
  const ProgramState next = nextProgramState(m_programState, x1);
  if (next != m_programState) {
    m_programState = next;
    values.insert("robotProgramState", static_cast<int>(next));
  }

  // The same rule as the PLC's ResumeBlocked: a program held by APPL_RUN while AUT EXT dropped
  // (an operating-mode change) keeps velocities planned in the other mode, so it is restarted from
  // CELL, not resumed. Cleared when APPL_RUN falls.
  const bool blocked = x1.value(X1ApplRun).toBool() && (m_resumeBlocked || !x1.value(X1AutExt).toBool());
  if (blocked != m_resumeBlocked) {
    m_resumeBlocked = blocked;
    values.insert("robotResumeBlocked", blocked);
  }

  emit stateReady(values);
}

PlcDevice::ProgramState PlcDevice::nextProgramState(ProgramState current, const QVariantList& x1)
{
  // P00 sets APPL_RUN at the program-number acknowledgement and clears it only when CELL asks for
  // the next number, so APPL_RUN without $PRO_ACT is a stopped (interrupted) application program.
  const bool proAct = x1.value(X1ProAct).toBool();
  const bool pgnoReq = x1.value(X1PgnoReq).toBool();
  const bool applRun = x1.value(X1ApplRun).toBool();

  if (applRun) return proAct ? ProgramState::Running : ProgramState::Interrupted;

  const bool started = current == ProgramState::Running || current == ProgramState::Interrupted;
  if (started && proAct && pgnoReq) return ProgramState::Finished;
  // KSS Fig. 6-25 ... 6-28 draw APPL_RUN falling on a stop; this covers that behaviour too.
  if (current == ProgramState::Running && !proAct) return ProgramState::Interrupted;
  return current;
}

QByteArray PlcDevice::swapBytes(const QByteArray& data)
{
  QByteArray out = data;

  for (int i = 0; i + 1 < out.size(); i += 2)
    qSwap(out[i], out[i + 1]);

  return out;
}
