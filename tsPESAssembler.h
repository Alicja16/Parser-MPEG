#pragma once

#include "tsCommon.h"
#include "tsTS.h"
#include "tsTransportStreamHeader.h"
#include "tsTransportStreamAF.h"
#include "tsPESPacketHeader.h"

#include <cstdint>

//=============================================================================================================================================================================
// PES Assembler
//
// - filtrowanie pakietów TS po wybranym PID
// - sprawdzanie continuity_counter
// - wyciąganie payloadu TS z uwzględnieniem Adaptation Field
// - składanie pełnego pakietu PES z wielu pakietów TS
// - udostępnianie całego PES albo samych PES_packet_data_bytes
//
//=============================================================================================================================================================================

class xPES_Assembler
{
public:

  enum class eResult : int32_t
  {
    UnexpectedPID = 1,
    StreamPacketLost,
    AssemblingStarted,
    AssemblingContinue,
    AssemblingFinished,
  };

protected:

  // setup
  int32_t m_PID;

  // buffer for currently assembled PES packet
  uint8_t* m_Buffer;
  uint32_t m_BufferSize;
  uint32_t m_DataOffset;

  // operation state
  int8_t m_LastContinuityCounter;
  bool   m_Started;

  // PES header parsed from current PES packet
  xPES_PacketHeader m_PESH;

public:

  xPES_Assembler();
  ~xPES_Assembler();

  void Init(int32_t PID);

  eResult AbsorbPacket(
    const uint8_t* TransportStreamPacket,
    const xTS_PacketHeader* PacketHeader,
    const xTS_AdaptationField* AdaptationField
  );

  void PrintResult(eResult Result) const;
  void PrintPESH() const { m_PESH.Print(); }

public:

  // Whole assembled PES packet
  uint8_t* getPacket() { return m_Buffer; }
  const uint8_t* getPacket() const { return m_Buffer; }

  int32_t getNumPacketBytes() const { return int32_t(m_DataOffset); }

  // Parsed PES header
  const xPES_PacketHeader& getPESH() const { return m_PESH; }

  // Pointer to PES_packet_data_bytes, czyli dane bez nagłówka PES
  const uint8_t* getPacketData() const
  {
    if(m_Buffer == nullptr)
    {
      return nullptr;
    }

    if(m_DataOffset < m_PESH.getHeaderLength())
    {
      return nullptr;
    }

    return m_Buffer + m_PESH.getHeaderLength();
  }

  // Length of PES_packet_data_bytes
  uint32_t getPacketDataLength() const
  {
    const uint32_t HeaderLength = m_PESH.getHeaderLength();

    if(m_DataOffset < HeaderLength)
    {
      return 0;
    }

    const uint32_t LengthFromBuffer = m_DataOffset - HeaderLength;
    const uint32_t LengthFromHeader = m_PESH.getDataLength();

    // For normal audio PES, PES_packet_length is known and LengthFromHeader is exact.
    if(LengthFromHeader != 0 && LengthFromHeader < LengthFromBuffer)
    {
      return LengthFromHeader;
    }

    // For PES_packet_length == 0, use what was assembled.
    return LengthFromBuffer;
  }

protected:

  void xBufferReset();
  void xBufferAppend(const uint8_t* Data, int32_t Size);
};

//=============================================================================================================================================================================