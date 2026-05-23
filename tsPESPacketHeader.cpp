#include "tsPESPacketHeader.h"
#include <cstdio>
#include <cinttypes>

//=============================================================================================================================================================================
// xPES_PacketHeader
//=============================================================================================================================================================================

void xPES_PacketHeader::Reset()
{
  // Mandatory PES header
  m_PacketStartCodePrefix = 0;
  m_StreamId = 0;
  m_PacketLength = 0;

  // Optional PES header - byte 6
  m_PESScramblingControl = 0;
  m_PESPriority = 0;
  m_DataAlignmentIndicator = 0;
  m_Copyright = 0;
  m_OriginalOrCopy = 0;

  // Optional PES header - byte 7
  m_PTSDTSFlags = 0;
  m_ESCRFlag = 0;
  m_ESRateFlag = 0;
  m_DSMTrickModeFlag = 0;
  m_AdditionalCopyInfoFlag = 0;
  m_PESCRCFlag = 0;
  m_PESExtensionFlag = 0;

  // Optional PES header - byte 8
  m_PESHeaderDataLength = 0;

  // Timing
  m_PTS = 0;
  m_DTS = 0;
  m_HasPTS = false;
  m_HasDTS = false;

  // Derived values
  m_HeaderLength = xTS::PES_HeaderLength;
}

//=============================================================================================================================================================================

bool xPES_PacketHeader::xHasOptionalPESHeader() const
{
  // According to PES syntax, these stream_id values do not have
  // the usual optional PES header with '10', flags and PES_header_data_length.

  if(m_StreamId == eStreamId_program_stream_map)       { return false; }
  if(m_StreamId == eStreamId_padding_stream)           { return false; }
  if(m_StreamId == eStreamId_private_stream_2)         { return false; }
  if(m_StreamId == eStreamId_ECM)                      { return false; }
  if(m_StreamId == eStreamId_EMM)                      { return false; }
  if(m_StreamId == eStreamId_program_stream_directory) { return false; }
  if(m_StreamId == eStreamId_DSMCC_stream)             { return false; }
  if(m_StreamId == eStreamId_ITUT_H222_1_type_E)       { return false; }

  return true;
}


//=============================================================================================================================================================================

bool xPES_PacketHeader::xIsTimestampValid(const uint8_t* Data, uint8_t ExpectedPrefix) const
{
  if(Data == nullptr)
  {
    return false;
  }

  const uint8_t Prefix = uint8_t((Data[0] & 0xF0) >> 4);

  if(Prefix != ExpectedPrefix)
  {
    return false;
  }

  // Marker bits must be equal to 1.
  if((Data[0] & 0x01) != 0x01) { return false; }
  if((Data[2] & 0x01) != 0x01) { return false; }
  if((Data[4] & 0x01) != 0x01) { return false; }

  return true;
}

//=============================================================================================================================================================================

uint64_t xPES_PacketHeader::xParseTimestamp(const uint8_t* Data) const
{
  // PTS/DTS has 33 useful bits stored in 5 bytes:
  //
  // xxxx  PTS[32..30] marker
  // PTS[29..22]
  // PTS[21..15] marker
  // PTS[14..7]
  // PTS[6..0] marker

  const uint64_t Timestamp =
      (uint64_t(Data[0] & 0x0E) << 29)
    | (uint64_t(Data[1])        << 22)
    | (uint64_t(Data[2] & 0xFE) << 14)
    | (uint64_t(Data[3])        <<  7)
    | (uint64_t(Data[4] & 0xFE) >>  1);

  return Timestamp;
}

//=============================================================================================================================================================================

int32_t xPES_PacketHeader::Parse(const uint8_t* Input)
{
  if(Input == nullptr)
  {
    return NOT_VALID;
  }

  Reset();

  //===========================================================================================================================================================================
  // Mandatory PES header - 6 bytes
  //
  // packet_start_code_prefix : 24 bits
  // stream_id                :  8 bits
  // PES_packet_length        : 16 bits
  //===========================================================================================================================================================================

  m_PacketStartCodePrefix =
      (uint32_t(Input[0]) << 16)
    | (uint32_t(Input[1]) <<  8)
    |  uint32_t(Input[2]);

  m_StreamId = Input[3];

  m_PacketLength =
      (uint16_t(Input[4]) << 8)
    |  uint16_t(Input[5]);

  if(m_PacketStartCodePrefix != 0x000001)
  {
    return NOT_VALID;
  }

  // Some PES stream_id values contain only the 6-byte basic header.
  if(!xHasOptionalPESHeader())
  {
    m_HeaderLength = xTS::PES_HeaderLength;
    return int32_t(m_HeaderLength);
  }

  //===========================================================================================================================================================================
  // Normal PES optional header starts after 6 bytes.
  //
  // Input[6]:
  // '10'                         2 bits
  // PES_scrambling_control       2 bits
  // PES_priority                 1 bit
  // data_alignment_indicator     1 bit
  // copyright                    1 bit
  // original_or_copy             1 bit
  //
  // Input[7]:
  // PTS_DTS_flags                2 bits
  // ESCR_flag                    1 bit
  // ES_rate_flag                 1 bit
  // DSM_trick_mode_flag          1 bit
  // additional_copy_info_flag    1 bit
  // PES_CRC_flag                 1 bit
  // PES_extension_flag           1 bit
  //
  // Input[8]:
  // PES_header_data_length       8 bits
  //===========================================================================================================================================================================

  const uint8_t Byte6 = Input[6];

  const uint8_t FixedBits = uint8_t((Byte6 & 0xC0) >> 6);

  if(FixedBits != 0b10)
  {
    return NOT_VALID;
  }

  m_PESScramblingControl   = uint8_t((Byte6 & 0x30) >> 4);
  m_PESPriority            = uint8_t((Byte6 & 0x08) >> 3);
  m_DataAlignmentIndicator = uint8_t((Byte6 & 0x04) >> 2);
  m_Copyright              = uint8_t((Byte6 & 0x02) >> 1);
  m_OriginalOrCopy         = uint8_t( Byte6 & 0x01);

  const uint8_t Byte7 = Input[7];

  m_PTSDTSFlags            = uint8_t((Byte7 & 0xC0) >> 6);
  m_ESCRFlag               = uint8_t((Byte7 & 0x20) >> 5);
  m_ESRateFlag             = uint8_t((Byte7 & 0x10) >> 4);
  m_DSMTrickModeFlag       = uint8_t((Byte7 & 0x08) >> 3);
  m_AdditionalCopyInfoFlag = uint8_t((Byte7 & 0x04) >> 2);
  m_PESCRCFlag             = uint8_t((Byte7 & 0x02) >> 1);
  m_PESExtensionFlag       = uint8_t( Byte7 & 0x01);

  m_PESHeaderDataLength = Input[8];

  // Full PES header length for normal audio/video PES:
  // 6 bytes mandatory header + 3 bytes optional header base + PES_header_data_length.
  m_HeaderLength = xTS::PES_HeaderLength + 3 + m_PESHeaderDataLength;

  //===========================================================================================================================================================================
  // Optional timing fields - PTS/DTS
  //
  // PTS_DTS_flags:
  // 00 - no PTS/DTS
  // 01 - forbidden
  // 10 - PTS only
  // 11 - PTS and DTS
  //===========================================================================================================================================================================

  uint32_t Offset = xTS::PES_HeaderLength + 3;
  uint32_t HeaderBytesLeft = m_PESHeaderDataLength;

  if(m_PTSDTSFlags == 0b01)
  {
    return NOT_VALID;
  }

  if(m_PTSDTSFlags == 0b10)
{
  if(HeaderBytesLeft < 5)
  {
    return NOT_VALID;
  }

  // PTS only: prefix should be '0010'
  if(!xIsTimestampValid(Input + Offset, 0b0010))
  {
    return NOT_VALID;
  }

  m_PTS = xParseTimestamp(Input + Offset);
  m_HasPTS = true;

  Offset += 5;
  HeaderBytesLeft -= 5;
}
else if(m_PTSDTSFlags == 0b11)
{
  if(HeaderBytesLeft < 10)
  {
    return NOT_VALID;
  }

  // PTS when DTS is also present: prefix should be '0011'
  if(!xIsTimestampValid(Input + Offset, 0b0011))
  {
    return NOT_VALID;
  }

  m_PTS = xParseTimestamp(Input + Offset);
  m_HasPTS = true;

  Offset += 5;
  HeaderBytesLeft -= 5;

  // DTS prefix should be '0001'
  if(!xIsTimestampValid(Input + Offset, 0b0001))
  {
    return NOT_VALID;
  }

  m_DTS = xParseTimestamp(Input + Offset);
  m_HasDTS = true;

  Offset += 5;
  HeaderBytesLeft -= 5;
}

  // Other optional fields, such as ESCR, ES_rate, DSM_trick_mode, etc.,
  // are not parsed in detail in this stage.
  // They are still safely skipped because m_HeaderLength includes
  // the whole PES_header_data_length.

  (void)Offset;
  (void)HeaderBytesLeft;

  return int32_t(m_HeaderLength);
}

//=============================================================================================================================================================================

void xPES_PacketHeader::Print() const
{
  printf("PES: PSCP=%u SID=%u L=%u HeadLen=%u DataLen=%u",
         m_PacketStartCodePrefix,
         unsigned(m_StreamId),
         unsigned(m_PacketLength),
         unsigned(m_HeaderLength),
         unsigned(getDataLength()));

  if(m_HasPTS)
  {
    const double PTSTime = double(m_PTS) / double(xTS::BaseClockFrequency_Hz);

    printf(" PTS=%" PRIu64 " Time=%0.6fs", m_PTS, PTSTime);
  }

  if(m_HasDTS)
  {
    const double DTSTime = double(m_DTS) / double(xTS::BaseClockFrequency_Hz);

    printf(" DTS=%" PRIu64 " Time=%0.6fs", m_DTS, DTSTime);
  }
}

//=============================================================================================================================================================================