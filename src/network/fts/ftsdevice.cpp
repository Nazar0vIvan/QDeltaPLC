// ftsdevice.cpp

#include "network/fts/ftsdevice.h"
#include "network/common/socketconfigutils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>
#include <QSaveFile>
#include <QTimer>
#include <QUdpSocket>
#include <QUuid>
#include <QtEndian>

#include <chrono>
#include <cstdlib>
#include <optional>
#include <utility>

namespace {

constexpr double kSampleHz = 7000.0;
constexpr double kDt = 1.0 / kSampleHz;
constexpr double kCount = 1'000'000.0;
constexpr double kAxisTol = 0.05;

constexpr int kReqLen = 8;
constexpr int kRespLen = 36;
constexpr int kBatchMs = 16;
constexpr int kRxTimeout = 300;
constexpr int kLogCap = 7500;

constexpr quint16 kHeader = 0x1234;
constexpr quint16 kStartCmd = 0x0002;
constexpr quint16 kStopCmd = 0x0000;
constexpr quint16 kBiasCmd = 0x0042;

bool acceptAxis(qint32& dst, qint32 src)
{
	const qint64 diff = qint64(src) - qint64(dst);
	const qint64 tol = static_cast<qint64>(kAxisTol * kCount);

	if (std::llabs(diff) < tol) return false;

	dst = src;
	return true;
}

std::optional<RDTResponse> parseResponse(const QByteArray& data)
{
	if (data.size() < kRespLen) return std::nullopt;

	const auto* raw = reinterpret_cast<const uchar*>(data.constData());

	RDTResponse sample;

	sample.rdt_sequence = qFromBigEndian<quint32>(raw + 0);
	sample.ft_sequence = qFromBigEndian<quint32>(raw + 4);
	sample.status = qFromBigEndian<quint32>(raw + 8);

	sample.Fx = qFromBigEndian<qint32>(raw + 12);
	sample.Fy = qFromBigEndian<qint32>(raw + 16);
	sample.Fz = qFromBigEndian<qint32>(raw + 20);

	sample.Tx = qFromBigEndian<qint32>(raw + 24);
	sample.Ty = qFromBigEndian<qint32>(raw + 28);
	sample.Tz = qFromBigEndian<qint32>(raw + 32);

	return sample;
}

} // namespace

FtsDevice::FtsDevice(const QString& name, QObject* parent) : AbstractDevice(name, parent) {}

void FtsDevice::startDevice()
{
	Q_ASSERT(!m_sock);
	Q_ASSERT(!m_rxTimeout);
	Q_ASSERT(!m_savePoll);

	m_sock = new QUdpSocket(this);
	m_rxTimeout = new QTimer(this);
	m_rxTimeout->setSingleShot(true);
	m_rxTimeout->setInterval(kRxTimeout);
	m_savePoll = new QTimer(this);
	m_savePoll->setInterval(kBatchMs);

	attachSocket(m_sock);

	QObject::connect(m_sock, &QUdpSocket::readyRead, this, &FtsDevice::onReadyRead);
	QObject::connect(m_rxTimeout, &QTimer::timeout, this, &FtsDevice::onRxTimeout);
	QObject::connect(m_savePoll, &QTimer::timeout, this, &FtsDevice::onLogSaveFinished);

  emit stateReady({
    {"fx", 0.0},
    {"fy", 0.0},
    {"fz", 0.0},
    {"tx", 0.0},
    {"ty", 0.0},
    {"tz", 0.0}
  });
	publishControlState();
}

void FtsDevice::stopDevice()
{
	if (!m_sock) return;

	disconnect();
	if (m_saveFuture.valid()) {
		m_saveFuture.wait();
		onLogSaveFinished();
	}

	delete std::exchange(m_savePoll, nullptr);
	delete std::exchange(m_rxTimeout, nullptr);
	delete std::exchange(m_sock, nullptr);
}

void FtsDevice::connect(const QVariantMap& config)
{
	if (!m_sock) {
		emit logMessage({"FTS device is not started", 0, objectName()});
		return;
	}

	if (!config.isEmpty()) {
		const QHostAddress localAddr(config.value("localAddress").toString());
		const QHostAddress peerAddr(config.value("peerAddress").toString());

		const auto localPort = parseSocketPort(config.value("localPort"));
		const auto peerPort = parseSocketPort(config.value("peerPort"));

		if (localAddr.isNull() || peerAddr.isNull()
				|| !localPort || !peerPort) {
			emit logMessage({"Invalid socket configuration", 0, objectName()});
			return;
		}

		if (m_sock->state() != QAbstractSocket::UnconnectedState) disconnect();
		m_la = localAddr;
		m_lp = *localPort;
		m_pa = peerAddr;
		m_pp = *peerPort;
	}

	if (m_la.isNull() || m_lp == 0 || m_pa.isNull() || m_pp == 0) {
		emit logMessage({"Socket configuration is incomplete", 0, objectName()});
		return;
	}

	if (m_sock->state() != QAbstractSocket::UnconnectedState) disconnect();

	emit stateReady({{"connectionConfig", QVariantMap{
		{"localAddress", m_la.toString()}, {"localPort", m_lp},
		{"peerAddress", m_pa.toString()}, {"peerPort", m_pp}
	}}});

	if (!m_sock->bind(m_la, m_lp, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
		emit logMessage({
			QString("Bind failed: %1").arg(m_sock->errorString()),
			0,
			objectName()
		});
		return;
	}
	publishControlState();

	emit logMessage({
		QString("Socket connected:<br/>"
						"&nbsp;&nbsp;Local: &nbsp;[%1] : [%2]<br/>"
						"&nbsp;&nbsp;Peer: &nbsp;&nbsp;[%3] : [%4]")
						.arg(m_la.toString())
						.arg(m_lp)
						.arg(m_pa.toString())
						.arg(m_pp),
		1,
		objectName()
	});
}

void FtsDevice::disconnect()
{
	if (!m_sock) return;

	if (m_sock->state() == QAbstractSocket::BoundState && !m_pa.isNull() && m_pp != 0) {
		stopStreaming();
	}

	m_sock->close();
	m_rxTimeout->stop();

	m_batch.clear();
	m_needBase = true;
	m_hasPub = false;
	m_streaming = false;
	m_streamRequested = false;
	m_startPending = false;
	m_stopRequested = false;

	stopLogRecording();

	publishControlState();
	emit logMessage({"Socket disconnected", 1, objectName()});
}

void FtsDevice::startStreaming()
{
	if (!m_sock || m_sock->state() != QAbstractSocket::BoundState) {
		emit logMessage({"FTS socket is not bound", 0, objectName()});
		return;
	}

	if (m_pa.isNull() || m_pp == 0) {
		emit logMessage({"FTS peer is not configured", 0, objectName()});
		return;
	}
	if (m_saveFuture.valid()) {
		emit logMessage({"Wait for the FTS recording to finish saving", 0, objectName()});
		return;
	}
	if (m_startPending || (m_streamRequested && m_streaming)) return;
	if (!sendRequest(kStartCmd)) return;

	m_needBase = true;
	m_batch.clear();
	m_hasPub = false;
	m_streamRequested = true;
	m_startPending = true;
	m_stopRequested = false;
	m_rxTimeout->start();
	publishControlState();

	emit streamReset();
}

void FtsDevice::stopStreaming()
{
	if (!sendRequest(kStopCmd)) return;
	if (m_logEnabled) processBatch();
	m_streamRequested = false;
	m_startPending = false;
	m_stopRequested = true;
	stopLogRecording();
	m_rxTimeout->start();
	publishControlState();

	emit streamReset();
}

void FtsDevice::bias()
{
	if (!isSocketReady() || !m_streaming || m_startPending || m_stopRequested) {
		emit logMessage({"Start the connected FTS stream before biasing", 0, objectName()});
		return;
	}
	if (!sendRequest(kBiasCmd)) return;
	m_batch.clear();
	m_hasPub = false;

	if (m_batchClock.isValid()) m_batchClock.restart();

	emit streamReset();
}

void FtsDevice::onReadyRead()
{
	bool receivedSample = false;
	while (m_sock && m_sock->hasPendingDatagrams()) {
		const QNetworkDatagram datagram = m_sock->receiveDatagram(m_sock->pendingDatagramSize());
		if (datagram.senderAddress() != m_pa || datagram.senderPort() != m_pp) continue;

		const auto parsed = parseResponse(datagram.data());
		if (!parsed) continue;
		receivedSample = true;

		RDTResponse sample = *parsed;

		if (m_needBase) {
			m_baseSeq = sample.rdt_sequence;
			m_batch.clear();
			m_batchClock.start();
			m_needBase = false;
		}
		if (!m_streaming || m_startPending) {
			m_streaming = true;
			m_startPending = false;
			publishControlState();
		}

		sample.timestamp = double(quint32(sample.rdt_sequence - m_baseSeq)) * kDt;

		emit dataSampleHFReady(sample);

		m_batch.push_back(sample);

		if (m_batchClock.elapsed() >= kBatchMs)
			processBatch();
	}
	if (receivedSample) m_rxTimeout->start();
}

void FtsDevice::onRxTimeout()
{
	if (m_logEnabled) processBatch();
	if (m_startPending)
		emit logMessage({"No FTS data received after Start; check the sensor connection", 0, objectName()});
	m_batch.clear();
	m_needBase = true;
	m_hasPub = false;
	m_streaming = false;
	m_streamRequested = false;
	m_startPending = false;

	stopLogRecording();

	publishControlState();
}

bool FtsDevice::isSocketReady() const
{
	return m_sock && m_sock->state() == QAbstractSocket::BoundState
			&& !m_pa.isNull() && m_pp != 0;
}

bool FtsDevice::canRecord() const
{
	return isSocketReady() && m_streaming && m_streamRequested
			&& !m_startPending && !m_stopRequested && !m_logEnabled && !m_saveFuture.valid();
}

bool FtsDevice::canSave() const
{
	return isSocketReady() && m_stopRequested && !m_streaming && !m_startPending
			&& m_rxTimeout && !m_rxTimeout->isActive() && !m_logEnabled
			&& !m_log.isEmpty() && !m_saveFuture.valid();
}

void FtsDevice::publishControlState()
{
	emit stateReady({
		{"streaming", m_streaming},
		{"startPending", m_startPending},
		{"stopRequested", m_stopRequested},
		{"recording", m_logEnabled},
		{"saving", m_saveFuture.valid()},
		{"canRecord", canRecord()},
		{"canSave", canSave()}
	});
}

void FtsDevice::publishState(const RDTResponse& sample)
{
	if (!m_hasPub) {
		m_lastPub = sample;
		m_hasPub = true;

		emit stateReady({
			{"fx", sample.Fx / kCount},
			{"fy", sample.Fy / kCount},
			{"fz", sample.Fz / kCount},
			{"tx", sample.Tx / kCount},
			{"ty", sample.Ty / kCount},
			{"tz", sample.Tz / kCount}
		});

		return;
	}

	QVariantHash vals;

	if (acceptAxis(m_lastPub.Fx, sample.Fx))
		vals.insert("fx", m_lastPub.Fx / kCount);

	if (acceptAxis(m_lastPub.Fy, sample.Fy))
		vals.insert("fy", m_lastPub.Fy / kCount);

	if (acceptAxis(m_lastPub.Fz, sample.Fz))
		vals.insert("fz", m_lastPub.Fz / kCount);

	if (acceptAxis(m_lastPub.Tx, sample.Tx))
		vals.insert("tx", m_lastPub.Tx / kCount);

	if (acceptAxis(m_lastPub.Ty, sample.Ty))
		vals.insert("ty", m_lastPub.Ty / kCount);

	if (acceptAxis(m_lastPub.Tz, sample.Tz))
		vals.insert("tz", m_lastPub.Tz / kCount);

	if (!vals.isEmpty()) emit stateReady(vals);
}

bool FtsDevice::sendRequest(quint16 cmd, quint32 count)
{
	if (!isSocketReady()) {
		emit logMessage({"Connect the FTS socket before sending a command", 0, objectName()});
		return false;
	}

	QByteArray data(kReqLen, '\0');
	auto* raw = reinterpret_cast<uchar*>(data.data());

	qToBigEndian<quint16>(kHeader, raw + 0);
	qToBigEndian<quint16>(cmd, raw + 2);
	qToBigEndian<quint32>(count, raw + 4);

	if (m_sock->writeDatagram(data, m_pa, m_pp) != data.size()) {
		emit logMessage({QString("Failed to send FTS command: %1").arg(m_sock->errorString()),
				0, objectName()});
		return false;
	}
	return true;
}

void FtsDevice::processBatch()
{
	if (m_batch.isEmpty()) return;

	const RDTResponse sample = m_batch.back();

  if (m_logEnabled)
    appendLogSample(sample);

	emit dataBatchReady(m_batch);
	publishState(sample);

	m_batch.clear();
	m_batchClock.restart();
}

void FtsDevice::startLogRecording()
{
  if (m_logEnabled) return;

  if (!canRecord() || !m_rxTimeout || !m_rxTimeout->isActive()) {
    emit logMessage({
      "Cannot record FTS log: no data is being received",
      0,
      objectName()
    });
    return;
  }

  m_log.clear();
  m_log.reserve(kLogCap);
  m_batch.clear();
  m_batchClock.start();
  m_logEnabled = true;
  publishControlState();

  emit logMessage({
    QString("LF log recording started (capacity=%1 samples)").arg(kLogCap),
    1,
    objectName()
  });
}

void FtsDevice::stopLogRecording()
{
  if (!m_logEnabled) return;

  m_logEnabled = false;
  publishControlState();

  emit logMessage({
    "LF log recording stopped",
    1,
    objectName()
  });
}

void FtsDevice::appendLogSample(const RDTResponse& sample)
{
  m_log.push_back(sample);

  if (m_log.size() < kLogCap) return;

  emit logMessage({
      QString("LF log reached capacity (%1 samples), auto-stopping").arg(kLogCap),
      2,
      objectName()
  });

  stopLogRecording();
}

void FtsDevice::saveLogToDefaultFile()
{
	if (!canSave()) {
		emit logMessage({"Connect FTS, record samples and Stop before saving", 0, objectName()});
		return;
	}

	const QString fileName = QStringLiteral("fts_%1_%2.json")
			.arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")),
				 QUuid::createUuid().toString(QUuid::WithoutBraces));
	const QDir records(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("records")));
	saveLogToFileImpl(records.filePath(fileName));
}

void FtsDevice::saveLogToFileImpl(const QString& filePath)
{
	m_saveFuture = std::async(std::launch::async, &FtsDevice::writeLogFile, m_log, filePath);
	m_savePoll->start();
	publishControlState();
}

FtsDevice::LogSaveResult FtsDevice::writeLogFile(QVector<RDTResponse> samples, QString filePath)
{
	LogSaveResult result{std::move(filePath), {}, samples.size(), 0};
	if (!QDir().mkpath(QFileInfo(result.filePath).absolutePath())) {
		result.error = QString("Cannot create the FTS records directory for '%1'").arg(result.filePath);
		return result;
	}
	QJsonArray jsonSamples;

	for (const RDTResponse& sample : std::as_const(samples)) {
		const QJsonObject item{
			{"rdt_sequence", static_cast<qint64>(sample.rdt_sequence)},
			{"ft_sequence", static_cast<qint64>(sample.ft_sequence)},
			{"status", static_cast<qint64>(sample.status)},
			{"Fx", static_cast<qint64>(sample.Fx)},
			{"Fy", static_cast<qint64>(sample.Fy)},
			{"Fz", static_cast<qint64>(sample.Fz)},
			{"Tx", static_cast<qint64>(sample.Tx)},
			{"Ty", static_cast<qint64>(sample.Ty)},
			{"Tz", static_cast<qint64>(sample.Tz)},
			{"timestamp", sample.timestamp}
		};

		jsonSamples.append(item);
	}

	const QJsonObject meta{
		{"capacity", kLogCap},
		{"count", samples.size()},
		{"emit_interval_ms", kBatchMs},
		{"counts_per_unit", kCount},
		{"note", QStringLiteral("Low-frequency FTS samples.")}
	};

	const QJsonObject root{
		{"meta", meta},
		{"samples", jsonSamples}
	};

	const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);

	QSaveFile file(result.filePath);

	if (!file.open(QIODevice::WriteOnly)) {
		result.error = QString("Failed to write log file '%1': %2").arg(result.filePath, file.errorString());
		return result;
	}

	const qint64 written = file.write(json);

	if (written != json.size()) {
		result.error = QString("Partial write to '%1': wrote %2 of %3 bytes")
				.arg(result.filePath).arg(written).arg(json.size());
		return result;
	}
	if (!file.commit()) {
		result.error = QString("Failed to finish log file '%1': %2").arg(result.filePath, file.errorString());
		return result;
	}
	result.byteCount = written;
	return result;
}

void FtsDevice::onLogSaveFinished()
{
	if (!m_saveFuture.valid()) {
		m_savePoll->stop();
		return;
	}
	if (m_saveFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) return;
	const LogSaveResult result = m_saveFuture.get();
	m_savePoll->stop();
	publishControlState();
	if (!result.error.isEmpty()) {
		emit logMessage({result.error, 0, objectName()});
		return;
	}
	emit logMessage({
		QString("Saved LF log to '%1' (%2 samples, %3 bytes)")
			.arg(result.filePath)
			.arg(result.sampleCount)
			.arg(result.byteCount),
		1,
		objectName()
	});
}
