//=============================================================================================================================================================================
#include "tsTransportStreamAF.h"
#include "tsTS.h"
#include <iostream>

//=============================================================================================================================================================================
// xTS_AdaptationField
//=============================================================================================================================================================================

/// @brief Reset - reset all TS packet AdaptationField fields
void xTS_AdaptationField::Reset()
{
  // setup / derived values
  m_AdaptationFieldControl = 0;
  m_NumBytes = 0;
  m_StuffingBytes = 0;

  // mandatory fields
  m_AdaptationFieldLength = 0;
  m_DC = 0;
  m_RA = 0;
  m_SP = 0;
  m_PR = 0;
  m_OR = 0;
  m_SF = 0;
  m_TP = 0;
  m_EX = 0;

  // optional fields parsed in this stage
  m_PCR = 0;
  m_OPCR = 0;
}

/// @brief Read PCR/OPCR value from 6 bytes according to ISO/IEC 13818-1.
/// @return full 27 MHz timestamp: base * 300 + extension
static uint64_t xReadClockReference(const uint8_t* Data)
{
  const uint64_t Base =
    (uint64_t(Data[0]) << 25) |
    (uint64_t(Data[1]) << 17) |
    (uint64_t(Data[2]) <<  9) |
    (uint64_t(Data[3]) <<  1) |
    (uint64_t((Data[4] & 0x80) >> 7));

  const uint16_t Extension =
    (uint16_t(Data[4] & 0x01) << 8) |
    uint16_t(Data[5]);

  return Base * xTS::BaseToExtendedClockMultiplier + Extension;
}

/**
  @brief Parse adaptation field.
  @param PacketBuffer is pointer to buffer containing full TS packet.
  @param AdaptationFieldControl is value of Adaptation Field Control field of corresponding TS packet header.
  @return Number of parsed/skipped bytes occupied by Adaptation Field or -1 on failure.
*/
int32_t xTS_AdaptationField::Parse(const uint8_t* PacketBuffer, uint8_t AdaptationFieldControl)
{
  m_AdaptationFieldControl = AdaptationFieldControl;

  // Adaptation Field exists only for AFC == 2 or AFC == 3.
  if(AdaptationFieldControl != 2 && AdaptationFieldControl != 3)
  {
    m_NumBytes = 0;
    m_StuffingBytes = 0;
    return 0;
  }

  uint32_t Offset = xTS::TS_HeaderLength;

  m_AdaptationFieldLength = PacketBuffer[Offset];
  Offset += 1;

  // Total adaptation field size is 1 + adaptation_field_length.
  m_NumBytes = 1 + m_AdaptationFieldLength;

  // In a 188-byte TS packet, after 4-byte TS header there are 184 bytes left.
  if(m_NumBytes > (xTS::TS_PacketLength - xTS::TS_HeaderLength))
  {
    return NOT_VALID;
  }

  // adaptation_field_length == 0 means no flags and no optional fields.
  if(m_AdaptationFieldLength == 0)
  {
    m_StuffingBytes = 0;
    return m_NumBytes;
  }

  // First byte after adaptation_field_length is the flags byte.
  const uint8_t Flags = PacketBuffer[Offset];
  Offset += 1;

  m_DC = uint8_t((Flags & 0x80) >> 7);
  m_RA = uint8_t((Flags & 0x40) >> 6);
  m_SP = uint8_t((Flags & 0x20) >> 5);
  m_PR = uint8_t((Flags & 0x10) >> 4);
  m_OR = uint8_t((Flags & 0x08) >> 3);
  m_SF = uint8_t((Flags & 0x04) >> 2);
  m_TP = uint8_t((Flags & 0x02) >> 1);
  m_EX = uint8_t( Flags & 0x01);

  uint32_t BytesLeft = m_AdaptationFieldLength - 1;

  // PCR: 6 bytes
  if(m_PR)
  {
    if(BytesLeft < 6) { return NOT_VALID; }

    m_PCR = xReadClockReference(&PacketBuffer[Offset]);
    Offset += 6;
    BytesLeft -= 6;
  }

  // OPCR: 6 bytes
  if(m_OR)
  {
    if(BytesLeft < 6) { return NOT_VALID; }

    m_OPCR = xReadClockReference(&PacketBuffer[Offset]);
    Offset += 6;
    BytesLeft -= 6;
  }

  // The following optional fields are not parsed in detail in this stage,
  // but must be skipped correctly so stuffing bytes are counted correctly.

  // splice_countdown: 1 byte
  if(m_SF)
  {
    if(BytesLeft < 1) { return NOT_VALID; }

    Offset += 1;
    BytesLeft -= 1;
  }

  // transport_private_data_length + private_data bytes
  if(m_TP)
  {
    if(BytesLeft < 1) { return NOT_VALID; }

    const uint8_t TransportPrivateDataLength = PacketBuffer[Offset];
    Offset += 1;
    BytesLeft -= 1;

    if(BytesLeft < TransportPrivateDataLength) { return NOT_VALID; }

    Offset += TransportPrivateDataLength;
    BytesLeft -= TransportPrivateDataLength;
  }

  // adaptation_field_extension_length + extension bytes
  if(m_EX)
  {
    if(BytesLeft < 1) { return NOT_VALID; }

    const uint8_t AdaptationFieldExtensionLength = PacketBuffer[Offset];
    Offset += 1;
    BytesLeft -= 1;

    if(BytesLeft < AdaptationFieldExtensionLength) { return NOT_VALID; }

    Offset += AdaptationFieldExtensionLength;
    BytesLeft -= AdaptationFieldExtensionLength;
  }

  // Whatever remains inside adaptation_field_length is stuffing.
  m_StuffingBytes = BytesLeft;

  return m_NumBytes;
}

/// @brief Print all TS packet Adaptation Field fields parsed in this stage
void xTS_AdaptationField::Print() const
{
  printf(" AF: L=%3d DC=%d RA=%d SP=%d PR=%d OR=%d SF=%d TP=%d EX=%d Stuffing=%d",
         int(m_AdaptationFieldLength),
         int(m_DC),
         int(m_RA),
         int(m_SP),
         int(m_PR),
         int(m_OR),
         int(m_SF),
         int(m_TP),
         int(m_EX),
         int(m_StuffingBytes));

  if(m_PR)
  {
    const double PCRTime = double(m_PCR) / double(xTS::ExtendedClockFrequency_Hz);
    printf(" PCR=%" PRIu64 " Time=%0.6fs", m_PCR, PCRTime);
  }

  if(m_OR)
  {
    const double OPCRTime = double(m_OPCR) / double(xTS::ExtendedClockFrequency_Hz);
    printf(" OPCR=%" PRIu64 " Time=%0.6fs", m_OPCR, OPCRTime);
  }
}

//=============================================================================================================================================================================