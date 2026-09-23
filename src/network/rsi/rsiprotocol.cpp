#include "rsiprotocol.h"
#include <QLocale>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <cmath>

QByteArray RsiProtocol::encode(const RsiTxFrame& tx)
{
  QByteArray out;
  out.reserve(256);

  QXmlStreamWriter xml(&out);
  xml.setAutoFormatting(false);

  xml.writeStartElement("Sen");
  xml.writeAttribute("Type", "ImFree");

  xml.writeEmptyElement("RKorr");

	static const char* keys[6] = { "X", "Y", "Z", "A", "B", "C" };

  const QLocale locale = QLocale::c();

  for (int i = 0; i < 6; ++i) {
		xml.writeAttribute(QLatin1String(keys[i]), locale.toString(tx.corr[static_cast<size_t>(i)], 'g', 10));
  }

  xml.writeTextElement("IPOC", QString::number(tx.ipoc));
  xml.writeEndElement();

  return out;
}


namespace {

std::optional<std::array<double, 6>> readSix(const QXmlStreamAttributes& attributes, const std::array<const char*, 6>& keys)
{
  std::array<double, 6> values{};
  for (std::size_t i = 0; i < keys.size(); ++i) {
    bool ok = false;
    values[i] = attributes.value(QLatin1String(keys[i])).toDouble(&ok);
    if (!ok || !std::isfinite(values[i])) return std::nullopt;
  }
  return values;
}

} // namespace

std::optional<RsiProtocol::Response> RsiProtocol::decode(const QByteArray& data)
{
  QXmlStreamReader xml(data);
  if (!xml.readNextStartElement() || xml.name() != QLatin1String("Rob"))
    return std::nullopt;

  Response response;
  bool hasIpoc = false;
  bool hasAxes = false;
  bool hasCurrents = false;
  while (xml.readNextStartElement()) {
    const auto name = xml.name();
    if (name == QLatin1String("IPOC")) {
      if (hasIpoc) return std::nullopt;
      const QString text = xml.readElementText().trimmed();
      if (text.isEmpty()) return std::nullopt;
      for (const QChar ch : text) {
        if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return std::nullopt;
      }
      bool ok = false;
      response.ipoc = text.toULongLong(&ok);
      if (!ok) return std::nullopt;
      hasIpoc = true;
    } else if (name == QLatin1String("RIst")) {
      if (response.pose) return std::nullopt;
      response.pose = readSix(xml.attributes(), {"X", "Y", "Z", "A", "B", "C"});
      if (!response.pose) return std::nullopt;
      xml.skipCurrentElement();
    } else if (name == QLatin1String("AIPos") || name == QLatin1String("MACur")) {
      bool& seen = name == QLatin1String("AIPos") ? hasAxes : hasCurrents;
      if (seen || !readSix(xml.attributes(), {"A1", "A2", "A3", "A4", "A5", "A6"}))
        return std::nullopt;
      seen = true;
      xml.skipCurrentElement();
    } else {
      // Unconsumed protocol extensions remain allowed, but must be well-formed XML.
      xml.skipCurrentElement();
    }
  }
  // Reading only through </Rob> misses truncated documents and trailing garbage.
  while (!xml.atEnd()) xml.readNext();
  if (xml.hasError() || !hasIpoc) return std::nullopt;
  return response;
}

bool RsiProtocol::acceptsSender(const QHostAddress& configuredAddress, quint16 learnedPort,
                                const QHostAddress& senderAddress, quint16 senderPort)
{
  return !configuredAddress.isNull() && !senderAddress.isNull()
      && configuredAddress == senderAddress && senderPort != 0
      && (learnedPort == 0 || learnedPort == senderPort);
}
