#pragma once

#include <cstdint>

#include <QMetaType>
#include <QtQml/qqmlregistration.h>

struct RDTRequest
{
  uint16_t header = 0;
  uint16_t command = 0;
  uint32_t sampleCount = 0;
};

struct RDTResponse
{
  Q_GADGET
  QML_VALUE_TYPE(rdtResponse)

  Q_PROPERTY(uint32_t rdt_sequence MEMBER rdt_sequence)
  Q_PROPERTY(uint32_t ft_sequence MEMBER ft_sequence)
  Q_PROPERTY(uint32_t status MEMBER status)
  Q_PROPERTY(int32_t Fx MEMBER Fx)
  Q_PROPERTY(int32_t Fy MEMBER Fy)
  Q_PROPERTY(int32_t Fz MEMBER Fz)
  Q_PROPERTY(int32_t Tx MEMBER Tx)
  Q_PROPERTY(int32_t Ty MEMBER Ty)
  Q_PROPERTY(int32_t Tz MEMBER Tz)
  Q_PROPERTY(double timestamp MEMBER timestamp)

public:
  uint32_t rdt_sequence = 0;
  uint32_t ft_sequence = 0;
  uint32_t status = 0;
  int32_t Fx = 0;
  int32_t Fy = 0;
  int32_t Fz = 0;
  int32_t Tx = 0;
  int32_t Ty = 0;
  int32_t Tz = 0;

  double timestamp = 0.0;
};

Q_DECLARE_METATYPE(RDTResponse)
