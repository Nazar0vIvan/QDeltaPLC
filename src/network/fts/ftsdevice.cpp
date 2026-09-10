// ftsdevice.cpp

#include "network/fts/ftsdevice.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>
#include <QTimer>
#include <QUdpSocket>
#include <QtEndian>

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

	m_sock = new QUdpSocket(this);
	m_rxTimeout = new QTimer(this);
	m_rxTimeout->setSingleShot(true);
	m_rxTimeout->setInterval(kRxTimeout);

	attachSocket(m_sock);

	QObject::connect(m_sock, &QUdpSocket::readyRead, this, &FtsDevice::onReadyRead);
	QObject::connect(m_rxTimeout, &QTimer::timeout, this, &FtsDevice::onRxTimeout);

  emit stateReady({
    {"streaming", false},
    {"fx", 0.0},
    {"fy", 0.0},
    {"fz", 0.0},
    {"tx", 0.0},
    {"ty", 0.0},
    {"tz", 0.0}
  });
}

void FtsDevice::stopDevice()
{
	if (!m_sock) return;

	disconnect();

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

		bool localPortOk = false;
		bool peerPortOk = false;
		const uint localPort = config.value("localPort").toUInt(&localPortOk);
		const uint peerPort = config.value("peerPort").toUInt(&peerPortOk);

		if (localAddr.isNull() || peerAddr.isNull()
				|| !localPortOk || !peerPortOk
				|| localPort == 0 || localPort > 65535
				|| peerPort == 0 || peerPort > 65535) {
			emit logMessage({"Invalid socket configuration", 0, objectName()});
			return;
		}

		m_la = localAddr;
		m_lp = static_cast<quint16>(localPort);
		m_pa = peerAddr;
		m_pp = static_cast<quint16>(peerPort);
	}

	if (m_la.isNull() || m_lp == 0 || m_pa.isNull() || m_pp == 0) {
		emit logMessage({"Socket configuration is incomplete", 0, objectName()});
		return;
	}

	if (m_sock->state() != QAbstractSocket::UnconnectedState) disconnect();

	if (!m_sock->bind(m_la, m_lp, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
		emit logMessage({
			QString("Bind failed: %1").arg(m_sock->errorString()),
			0,
			objectName()
		});
		return;
	}

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

	stopLogRecording();

	emit stateReady({{"streaming", false}});
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

	m_needBase = true;
	m_batch.clear();
	m_hasPub = false;

	emit streamReset();

	sendRequest(kStartCmd);
}

void FtsDevice::stopStreaming()
{
	stopLogRecording();
	sendRequest(kStopCmd);

	emit streamReset();
}

void FtsDevice::bias()
{
	m_batch.clear();
	m_hasPub = false;

	if (m_batchClock.isValid()) m_batchClock.restart();

	sendRequest(kBiasCmd);

	emit streamReset();
}

void FtsDevice::onReadyRead()
{
	while (m_sock && m_sock->hasPendingDatagrams()) {
		const QNetworkDatagram datagram = m_sock->receiveDatagram(m_sock->pendingDatagramSize());

		const auto parsed = parseResponse(datagram.data());
		if (!parsed) continue;

		RDTResponse sample = *parsed;

		if (m_needBase) {
			m_baseSeq = sample.rdt_sequence;
			m_batch.clear();
			m_batchClock.start();
			m_rxTimeout->start();
			m_needBase = false;

			emit stateReady({{"streaming", true}});
		}

		sample.timestamp = double(quint32(sample.rdt_sequence - m_baseSeq)) * kDt;

		emit dataSampleHFReady(sample);

		m_batch.push_back(sample);

		if (m_batchClock.elapsed() >= kBatchMs)
			processBatch();
	}
}

void FtsDevice::onRxTimeout()
{
	m_batch.clear();
	m_needBase = true;
	m_hasPub = false;

	stopLogRecording();

	emit stateReady({{"streaming", false}});
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

void FtsDevice::sendRequest(quint16 cmd, quint32 count)
{
	if (!m_sock || m_pa.isNull() || m_pp == 0) return;

	QByteArray data(kReqLen, '\0');
	auto* raw = reinterpret_cast<uchar*>(data.data());

	qToBigEndian<quint16>(kHeader, raw + 0);
	qToBigEndian<quint16>(cmd, raw + 2);
	qToBigEndian<quint32>(count, raw + 4);

	m_sock->writeDatagram(data, m_pa, m_pp);
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
	m_rxTimeout->start();
}

void FtsDevice::startLogRecording()
{
  if (m_logEnabled) return;

  if (!m_rxTimeout->isActive()) {
    emit logMessage({
      "Cannot record FTS log: no data is being received",
      0,
      objectName()
    });
    return;
  }

  m_log.clear();
  m_log.reserve(kLogCap);
  m_logEnabled = true;

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
	if (m_rxTimeout->isActive()) {
		emit logMessage({"Cannot save FTS log while data is being received", 0, objectName()});
		return;
	}

	saveLogToFileImpl(QStringLiteral("record.json"));
}

void FtsDevice::saveLogToFileImpl(const QString& filePath)
{
	QJsonArray samples;

	for (const RDTResponse& sample : std::as_const(m_log)) {
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

		samples.append(item);
	}

	const QJsonObject meta{
		{"capacity", kLogCap},
		{"count", m_log.size()},
		{"emit_interval_ms", kBatchMs},
		{"note", QStringLiteral("Low-frequency FTS samples.")}
	};

	const QJsonObject root{
		{"meta", meta},
		{"samples", samples}
	};

	const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);

	QFile file(filePath);

	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		emit logMessage({
			QString("Failed to write log file '%1': %2").arg(filePath, file.errorString()),
			0,
			objectName()
		});
		return;
	}

	const qint64 written = file.write(json);

	if (written != json.size()) {
		emit logMessage({
			QString("Partial write to '%1': wrote %2 of %3 bytes")
				.arg(filePath)
				.arg(written)
				.arg(json.size()),
			0,
			objectName()
		});
		return;
	}

	emit logMessage({
		QString("Saved LF log to '%1' (%2 samples, %3 bytes)")
			.arg(filePath)
			.arg(m_log.size())
			.arg(json.size()),
		1,
		objectName()
	});
}
