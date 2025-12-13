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
    emit logMessage({method + ": Socket is null", 0, ""});
    return;
  }

  const QByteArray name = method.toLatin1();
  const bool haveArgs = !args.isEmpty();

  int idx = -1; bool haveReturn = false;
  if (haveArgs) {
    idx = indexOfSignature(name + "(QVariantMap)");
    haveReturn = m_socket->metaObject()->method(idx).returnMetaType().id() == QMetaType::QVariantMap;
  }
  else {
    idx = indexOfSignature(name + "()");
  }

  if (idx == -1) {
    emit logMessage({method + ": no such method", 0, m_socket->objectName()});
  }

  bool ok = false; QVariantMap out;

  if (haveReturn) {
    ok = QMetaObject::invokeMethod(m_socket,
                                  name.constData(),
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QVariantMap, out),
                                  Q_ARG(QVariantMap, args));
  }
  else if (haveArgs) {
    ok = QMetaObject::invokeMethod(m_socket,
                                  name.constData(),
                                  Qt::QueuedConnection,
                                  Q_ARG(QVariantMap, args));
  }
  else {
    ok = QMetaObject::invokeMethod(m_socket,
                                  name.constData(),
                                  Qt::QueuedConnection);
  }

  if (ok) {
    emit resultReady(method, out);
  } else {
    emit logMessage({method + ": failed to invoke", 0, m_socket->objectName()});
  }
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
  connect(m_socket, &QIODevice::bytesWritten, this, &UdpSocketRunner::onPulse, Qt::QueuedConnection);
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

void FtsRunner::onDataBatchReady(const QVector<RDTResponse> &batch)
{
  if(batch.isEmpty()) return;

  // Take the last sample from the batch
  const RDTResponse &latest = batch.back();

  // First reading – just store and emit
  if (m_lastReading.isEmpty() || m_lastReading.size() != latest.size()) {
    m_lastReading = latest;
    emit lastReadingChanged();
    return;
  }

  if (m_tolerance <= 0.0) {
    m_lastReading = latest;
    emit lastReadingChanged();
    return;
  }

  static constexpr double COUNT_FACTOR = 1000000.0;
  const double tolCounts = m_tolerance * COUNT_FACTOR;

  QVariantList newReading = m_lastReading;
  bool anyChanged = false;

  const int firstIdx = 3;
  const int lastIdx  = 8;

  for (int i = firstIdx; i <= lastIdx && i < latest.size() && i < newReading.size(); ++i)
  {
    const double newVal = latest[i].toDouble();
    const double oldVal = newReading[i].toDouble();

    if (std::abs(newVal - oldVal) > tolCounts) {
      newReading[i] = latest[i];   // update only this component
      anyChanged    = true;
    }
  }

  // If all 6 components are within tolerance → do nothing
  if (!anyChanged) return;

  // Optionally keep meta fields (0..2) in sync with latest sample,
  // they are not used in QML bars, so this doesn’t affect UI values.
  const int metaCount = qMin(3, qMin(latest.size(), newReading.size()));
  for (int i = 0; i < metaCount; ++i)
    newReading[i] = latest[i];

  m_lastReading = newReading;
  emit lastReadingChanged();
}


// ----- RsiRunner -----

RsiRunner::RsiRunner(QAbstractSocket* socket, QObject* parent) : UdpSocketRunner(socket, parent)
{
  auto* rsi = qobject_cast<SocketRSI*>(socket);
  if (!rsi) {
    // You can log or assert here, this runner is meant only for SocketRSI
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

