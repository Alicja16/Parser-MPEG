#pragma once

#include "tsCommon.h"
#include "tsTS.h"
#include <cstdint>

//=============================================================================================================================================================================
// PES Packet Header
//
// - parsowanie nagłówka PES
// - odczyt podstawowych pól: packet_start_code_prefix, stream_id, PES_packet_length
// - odczyt opcjonalnego nagłówka PES dla normalnych strumieni audio/video
// - wyliczenie długości całego nagłówka PES
// - wyliczenie długości danych PES_packet_data_bytes
//
//=============================================================================================================================================================================

class xPES_PacketHeader
{
public:

  enum eStreamId : uint8_t
  {
    eStreamId_program_stream_map       = 0xBC,
    eStreamId_padding_stream           = 0xBE,
    eStreamId_private_stream_2         = 0xBF,
    eStreamId_ECM                      = 0xF0,
    eStreamId_EMM                      = 0xF1,
    eStreamId_program_stream_directory = 0xFF,
    eStreamId_DSMCC_stream             = 0xF2,
    eStreamId_ITUT_H222_1_type_E       = 0xF8,
  };

protected:

  //===========================================================================================================================================================================
  // Mandatory PES header - 6 bytes
  //===========================================================================================================================================================================

  uint32_t m_PacketStartCodePrefix;   // 24 bits, should be 0x000001
  uint8_t  m_StreamId;                //  8 bits
  uint16_t m_PacketLength;            // 16 bits, number of bytes after this field

  //===========================================================================================================================================================================
  // Optional PES header - byte 6
  //===========================================================================================================================================================================

  uint8_t m_PESScramblingControl;     // 2 bits
  uint8_t m_PESPriority;              // 1 bit
  uint8_t m_DataAlignmentIndicator;   // 1 bit
  uint8_t m_Copyright;                // 1 bit
  uint8_t m_OriginalOrCopy;           // 1 bit

  //===========================================================================================================================================================================
  // Optional PES header - byte 7
  //===========================================================================================================================================================================

  uint8_t m_PTSDTSFlags;              // 2 bits
  uint8_t m_ESCRFlag;                 // 1 bit
  uint8_t m_ESRateFlag;               // 1 bit
  uint8_t m_DSMTrickModeFlag;         // 1 bit
  uint8_t m_AdditionalCopyInfoFlag;   // 1 bit
  uint8_t m_PESCRCFlag;               // 1 bit
  uint8_t m_PESExtensionFlag;         // 1 bit

  //===========================================================================================================================================================================
  // Optional PES header - byte 8
  //===========================================================================================================================================================================

  uint8_t m_PESHeaderDataLength;      // 8 bits

  //===========================================================================================================================================================================
  // Optional timing fields - parsed, but not required for audio extraction
  //===========================================================================================================================================================================

  uint64_t m_PTS;
  uint64_t m_DTS;
  bool     m_HasPTS;
  bool     m_HasDTS;

  //===========================================================================================================================================================================
  // Derived values
  //===========================================================================================================================================================================

  uint32_t m_HeaderLength;            // length from packet_start_code_prefix to PES_packet_data_bytes

public:

  void    Reset();
  int32_t Parse(const uint8_t* Input);
  void    Print() const;

protected:

  bool     xHasOptionalPESHeader() const;
  uint64_t xParseTimestamp(const uint8_t* Data) const;

public:

  // Mandatory PES header
  uint32_t getPacketStartCodePrefix() const { return m_PacketStartCodePrefix; }
  uint8_t  getStreamId()               const { return m_StreamId; }
  uint16_t getPacketLength()           const { return m_PacketLength; }

  // Optional PES header
  uint8_t getPESHeaderDataLength()     const { return m_PESHeaderDataLength; }
  uint8_t getPTSDTSFlags()             const { return m_PTSDTSFlags; }

  uint8_t getPESScramblingControl()    const { return m_PESScramblingControl; }
  uint8_t getPESPriority()             const { return m_PESPriority; }
  uint8_t getDataAlignmentIndicator()  const { return m_DataAlignmentIndicator; }
  uint8_t getCopyright()               const { return m_Copyright; }
  uint8_t getOriginalOrCopy()          const { return m_OriginalOrCopy; }

  uint8_t getESCRFlag()                const { return m_ESCRFlag; }
  uint8_t getESRateFlag()              const { return m_ESRateFlag; }
  uint8_t getDSMTrickModeFlag()        const { return m_DSMTrickModeFlag; }
  uint8_t getAdditionalCopyInfoFlag()  const { return m_AdditionalCopyInfoFlag; }
  uint8_t getPESCRCFlag()              const { return m_PESCRCFlag; }
  uint8_t getPESExtensionFlag()        const { return m_PESExtensionFlag; }

  // Timing
  bool     hasPTS()                    const { return m_HasPTS; }
  bool     hasDTS()                    const { return m_HasDTS; }
  uint64_t getPTS()                    const { return m_PTS; }
  uint64_t getDTS()                    const { return m_DTS; }

  // Derived values
  uint32_t getHeaderLength() const { return m_HeaderLength; }

  // Full PES packet length counted from packet_start_code_prefix.
  // PES_packet_length describes bytes after PES_packet_length field,
  // so full PES size = 6 + PES_packet_length.
  // If PES_packet_length == 0, size is unspecified.
  uint32_t getFullPacketLength() const
  {
    if(m_PacketLength == 0)
    {
      return 0;
    }

    return xTS::PES_HeaderLength + m_PacketLength;
  }

  // Length of PES_packet_data_bytes.
  // For PES_packet_length == 0 exact data length is unknown at header level.
  uint32_t getDataLength() const
  {
    const uint32_t FullPacketLength = getFullPacketLength();

    if(FullPacketLength == 0)
    {
      return 0;
    }

    if(FullPacketLength < m_HeaderLength)
    {
      return 0;
    }

    return FullPacketLength - m_HeaderLength;
  }
};

//=============================================================================================================================================================================