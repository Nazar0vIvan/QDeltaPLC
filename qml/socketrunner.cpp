#include "socketrunner.h"
#include "socketrsi.h"

// ----- AbstractSocketRunner -----

AbstractSocketRunner::AbstractSocketRunner(QAbstractSocket* socket, QObject* parent) : QObject(parent)
{
  m_thread = new QThread(this);
  m_thread->setObjectName(QStringLiteral("SocketRunnerThread"));

  attachSocket(socket);
  m_api = invokableMethodNames();

  connect(this, &AbstractSocketRunner::logMessage,  Logger::instance(), &Logger::push);
}

AbstractSocketRunner::~AbstractSocketRunner()
{
  // CAN'T just call m_socket destructor because it is in the working thread (managed by m_thread)
  // m_thread itself deletes via parenting to AbstractSocketRunner
  stop(); // this is a fallback in case if runner is destroyed manually (not by app closing)
}

bool AbstractSocketRunner::allowed(const QString &methodName) const
{
  return m_api.contains(methodName);
}

int AbstractSocketRunner::indexOfSignature(const QByteArray& sig) const {
  if (!m_socket) return -1;
  const QByteArray norm = QMetaObject::normalizedSignature(sig.constData());
  for (auto m = m_socket->metaObject(); m; m = m->superClass()) {
    const int idx = m->indexOfMethod(norm.constData());
    if (idx >= 0) return idx;
  }
  return -1;
}

void AbstractSocketRunner::invoke(const QString& method, const QVariantMap& args)
{
  if (!m_socket) {
    emit logMessage({method + ": socket is null", 0, ""});
    return;
  }

  // Enforce allow-list (your m_api is built from the socket's own public invokables/slots)
  if (!allowed(method)) {
    emit logMessage({method + ": not allowed", 0, m_socket->objectName()});
    return;
  }

  // If somebody calls invoke() before start(), ensure thread is running.
  if (m_thread && !m_thread->isRunning()) {
    m_thread->start();
  }

  const QByteArray name = method.toLatin1();
  const bool haveArgs = !args.isEmpty();

  // Find the method index (signature: () or (QVariantMap))
  int idx = -1;
  if (haveArgs) {
    idx = indexOfSignature(name + "(QVariantMap)");
  } else {
    idx = indexOfSignature(name + "()");
    if (idx < 0) {
      // allow calling (QVariantMap) with empty args if that's the only signature
      idx = indexOfSignature(name + "(QVariantMap)");
    }
  }

  if (idx < 0) {
    emit logMessage({method + ": no such invokable signature", 0, m_socket->objectName()});
    return;
  }

  const QMetaMethod mm = m_socket->metaObject()->method(idx);
  const int retTypeId = mm.returnMetaType().id();
  const bool returnsMap  = (retTypeId == QMetaType::QVariantMap);
  const bool returnsVoid = (retTypeId == QMetaType::Void);

  if (!returnsVoid && !returnsMap) {
    emit logMessage({method + ": unsupported return type (only void/QVariantMap allowed)", 0, m_socket->objectName()});
    return;
  }

  // if already on socket thread, do DirectConnection.
  const bool sameThread = (QThread::currentThread() == m_socket->thread());
  const Qt::ConnectionType ct =
      returnsMap ? (sameThread ? Qt::DirectConnection : Qt::BlockingQueuedConnection)
                 : (sameThread ? Qt::DirectConnection : Qt::QueuedConnection);

  bool ok = false;
  QVariantMap out;

  if (returnsMap) {
    ok = QMetaObject::invokeMethod(
        m_socket, name.constData(), ct,
        Q_RETURN_ARG(QVariantMap, out),
        Q_ARG(QVariantMap, args)
        );
  } else {
    if (mm.parameterCount() == 1) {
      ok = QMetaObject::invokeMethod(
          m_socket, name.constData(), ct,
          Q_ARG(QVariantMap, args)
          );
    } else {
      ok = QMetaObject::invokeMethod(m_socket, name.constData(), ct);
    }
  }

  if (ok) emit resultReady(method, out);
  else emit logMessage({method + ": invoke failed", 0, m_socket->objectName()});
}

void AbstractSocketRunner::start()
{
  if (!m_thread->isRunning())
    m_thread->start();
}

void AbstractSocketRunner::stop()
{
  // you can't directly access m_socket, it is in working thread
  // guard, if you accidentally call stop() second time it will return
  auto* sock = std::exchange(m_socket, nullptr);
  if (!sock) return;

  //  The following must be done:
  // - block Main Thread
  // - disconnect, close and delete socket in working thread
  // - finish working thread EL
  QMetaObject::invokeMethod(sock, [sock]{
      sock->disconnectFromHost();
      sock->close();
      delete sock;
  }, Qt::BlockingQueuedConnection); // blocks Main Thread during lambda execution
  m_thread->quit();
  m_thread->wait();
}

void AbstractSocketRunner::attachSocket(QAbstractSocket* sock)
{
  Q_ASSERT(sock);
  if (sock->parent()) sock->setParent(nullptr);

  m_socket = sock;

  m_socketState = m_socket->state();
  emit socketStateChanged();

  m_socket->moveToThread(m_thread); // !!!

  connect(m_thread, &QThread::started, this, &AbstractSocketRunner::onThreadStarted);
  connect(m_socket, &QAbstractSocket::stateChanged, this, &AbstractSocketRunner::onSocketStateChanged, Qt::QueuedConnection);
}

QStringList AbstractSocketRunner::invokableMethodNames() const
{
  QStringList out;
  if (!m_socket) return out;
  const QMetaObject* mo = m_socket->metaObject();
  for (int i = mo->methodOffset(); i < mo->methodCount(); ++i) {
    QMetaMethod mm = mo->method(i);
    if (mm.methodType() == QMetaMethod::Method && mm.access() == QMetaMethod::Public)
      out << QString::fromLatin1(mm.name());
  }
  out.removeDuplicates();
  std::sort(out.begin(), out.end());
  return out;
}

// SLOTS

void AbstractSocketRunner::onSocketStateChanged(QAbstractSocket::SocketState state)
{
  if (m_socketState == static_cast<int>(state)) return;
  m_socketState = static_cast<int>(state);
  emit socketStateChanged();
}

void AbstractSocketRunner::onThreadStarted()
{
  emit logMessage({"The thread has started", 1, m_socket->objectName()});
}

void AbstractSocketRunner::onThreadFinished()
{
  emit logMessage({"The thread has finished", 1, m_socket->objectName()});
}

// ----- TcpSocketRunner -----

TcpSocketRunner::TcpSocketRunner(QAbstractSocket* socket, QObject *parent) : AbstractSocketRunner(socket, parent)
{

}

TcpSocketRunner::~TcpSocketRunner() = default;

// ----- UdpSocketRunner -----

UdpSocketRunner::UdpSocketRunner(QAbstractSocket *socket, QObject *parent) : AbstractSocketRunner(socket, parent)
{
  m_timer.setSingleShot(true);
  m_timer.setInterval(300);
  connect(&m_timer, &QTimer::timeout, this, [this]{
    if (m_isStreaming) { m_isStreaming = false; emit isStreamingChanged(); }
  });

  connect(m_socket, &QIODevice::readyRead, this, &UdpSocketRunner::onPulse, Qt::QueuedConnection);
}

UdpSocketRunner::~UdpSocketRunner() = default;

void UdpSocketRunner::onPulse()
{
  if (!m_isStreaming) {
    m_isStreaming = true;
    emit isStreamingChanged();
  }
  m_timer.start(); // restart idle countdown on every pulse
}

// ----- FtsRunner -----

FtsRunner::FtsRunner(QAbstractSocket *socket, QObject *parent) : UdpSocketRunner(socket, parent)
{
  auto* fts = qobject_cast<SocketFTS*>(socket);
  if (!fts) {
    emit logMessage({"FtsRunner: socket is not SocketFTS", 0, objectName()});
    return;
  }

  connect(fts, &SocketFTS::dataSampleLFReady, this, &FtsRunner::onDataSampleLFReady, Qt::QueuedConnection);
}

double FtsRunner::axisValue(const QString &tag) const
{
  if (!m_hasPublished) return 0.0;

  if (tag == "Fx") return m_sample.Fx;
  if (tag == "Fy") return m_sample.Fy;
  if (tag == "Fz") return m_sample.Fz;
  if (tag == "Tx") return m_sample.Tx;
  if (tag == "Ty") return m_sample.Ty;
  if (tag == "Tz") return m_sample.Tz;
  return 0.0;
}

void FtsRunner::onDataSampleLFReady(const RDTResponse& s)
{
  if (!m_hasPublished) {
    m_lastPublished = s;
    m_hasPublished = true;
    publish(s);
    return;
  }

  RDTResponse out = m_lastPublished;
  copyMeta(out, s);

  static constexpr double COUNT_FACTOR = 1000000.0;
  const double tolCounts = m_tolerance * COUNT_FACTOR;
  if (!applyDeadbandAxes(out, s, tolCounts)) return;

  m_lastPublished = out;
  publish(out);
}

void FtsRunner::copyMeta(RDTResponse &dst, const RDTResponse &src)
{
  dst.rdt_sequence = src.rdt_sequence;
  dst.ft_sequence  = src.ft_sequence;
  dst.status       = src.status;
  dst.timestamp    = src.timestamp;
}

static inline bool acceptAxis(int32_t& dst, int32_t src, qint64 tolCounts)
{
  const qint64 d = qint64(src) - qint64(dst);
  if (std::llabs(d) >= tolCounts) { dst = src; return true; }
  return false;
}

bool FtsRunner::applyDeadbandAxes(RDTResponse &dst, const RDTResponse &src, qint64 tolCounts)
{
  bool any = false;
  any |= acceptAxis(dst.Fx, src.Fx, tolCounts);
  any |= acceptAxis(dst.Fy, src.Fy, tolCounts);
  any |= acceptAxis(dst.Fz, src.Fz, tolCounts);
  any |= acceptAxis(dst.Tx, src.Tx, tolCounts);
  any |= acceptAxis(dst.Ty, src.Ty, tolCounts);
  any |= acceptAxis(dst.Tz, src.Tz, tolCounts);
  return any;
}

void FtsRunner::publish(const RDTResponse &s)
{
  m_sample = s;
  emit sampleReady();
}



// ----- RsiRunner -----

RsiRunner::RsiRunner(QAbstractSocket* socket, QObject* parent) : UdpSocketRunner(socket, parent)
{
  auto* rsi = qobject_cast<SocketRSI*>(socket);
  if (!rsi) {
    emit logMessage({"RsiRunner: socket is not SocketRSI", 0, objectName()});
    return;
  }

  connect(rsi, &SocketRSI::motionStarted, this, &RsiRunner::onMotionStarted, Qt::QueuedConnection);
  connect(rsi, &SocketRSI::motionFinished, this, &RsiRunner::onMotionFinished, Qt::QueuedConnection);
  connect(rsi, &SocketRSI::motionActiveChanged, this, &RsiRunner::onMotionActiveChanged, Qt::QueuedConnection);
  connect(rsi, &SocketRSI::trajectoryReady, this, &RsiRunner::trajectoryReady, Qt::QueuedConnection);
}

void RsiRunner::onMotionStarted()
{
  m_motionActive = true;
  emit motionStarted();
  emit motionActiveChanged();
}

void RsiRunner::onMotionFinished()
{
  m_motionActive = false;
  emit motionFinished();
  emit motionActiveChanged();
}

void RsiRunner::onMotionActiveChanged(bool active)
{
  if (m_motionActive == active) return;
  m_motionActive = active;
  emit motionActiveChanged();
}

