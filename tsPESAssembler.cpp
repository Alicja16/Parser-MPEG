#include "tsPESAssembler.h"
#include <cstdio>
#include <cstring>

//=============================================================================================================================================================================
// xPES_Assembler
//=============================================================================================================================================================================

xPES_Assembler::xPES_Assembler()
{
  m_PID = -1;

  m_Buffer = nullptr;
  m_BufferSize = 0;
  m_DataOffset = 0;

  m_LastContinuityCounter = -1;
  m_Started = false;

  m_PESH.Reset();
}

//=============================================================================================================================================================================

xPES_Assembler::~xPES_Assembler()
{
  delete[] m_Buffer;
  m_Buffer = nullptr;
}

//=============================================================================================================================================================================

void xPES_Assembler::Init(int32_t PID)
{
  m_PID = PID;

  m_BufferSize = 1024 * 1024;

  delete[] m_Buffer;
  m_Buffer = new uint8_t[m_BufferSize];

  m_DataOffset = 0;
  m_LastContinuityCounter = -1;
  m_Started = false;

  m_PESH.Reset();
}

//=============================================================================================================================================================================

void xPES_Assembler::xBufferReset()
{
  m_DataOffset = 0;
  m_PESH.Reset();
}

//=============================================================================================================================================================================

void xPES_Assembler::xBufferAppend(const uint8_t* Data, int32_t Size)
{
  if(Data == nullptr || Size <= 0)
  {
    return;
  }

  if(m_Buffer == nullptr || m_BufferSize == 0)
  {
    m_BufferSize = 1024 * 1024;
    m_Buffer = new uint8_t[m_BufferSize];
  }

  if(m_DataOffset + uint32_t(Size) > m_BufferSize)
  {
    uint32_t NewBufferSize = m_BufferSize;

    while(m_DataOffset + uint32_t(Size) > NewBufferSize)
    {
      NewBufferSize *= 2;
    }

    uint8_t* NewBuffer = new uint8_t[NewBufferSize];

    if(m_Buffer != nullptr && m_DataOffset > 0)
    {
      std::memcpy(NewBuffer, m_Buffer, m_DataOffset);
    }

    delete[] m_Buffer;
    m_Buffer = NewBuffer;
    m_BufferSize = NewBufferSize;
  }

  std::memcpy(m_Buffer + m_DataOffset, Data, Size);
  m_DataOffset += uint32_t(Size);
}

//=============================================================================================================================================================================

xPES_Assembler::eResult xPES_Assembler::AbsorbPacket(
  const uint8_t* TransportStreamPacket,
  const xTS_PacketHeader* PacketHeader,
  const xTS_AdaptationField* AdaptationField
)
{
  if(TransportStreamPacket == nullptr || PacketHeader == nullptr)
  {
    return eResult::StreamPacketLost;
  }

  // Ten assembler obsługuje tylko jeden wybrany PID, np. PID=136 dla fonii.
  if(PacketHeader->getPacketIdentifier() != m_PID)
  {
    return eResult::UnexpectedPID;
  }

  // PES jest przenoszony tylko w payloadzie TS.
  if(!PacketHeader->hasPayload())
  {
    return eResult::AssemblingContinue;
  }

  //===========================================================================================================================================================================
  // Continuity Counter
  //
  // CC zwiększa się o 1 dla kolejnych pakietów z payloadem o tym samym PID.
  // Jeśli numer się nie zgadza, uznajemy, że pakiet został zgubiony.
  //===========================================================================================================================================================================

  const int8_t CurrentCC = int8_t(PacketHeader->getContinuityCounter());

  if(m_LastContinuityCounter >= 0)
  {
    const int8_t ExpectedCC = int8_t((m_LastContinuityCounter + 1) & 0x0F);

    if(CurrentCC == m_LastContinuityCounter)
    {
      // Duplikat pakietu — nie dopisujemy payloadu drugi raz.
      return eResult::AssemblingContinue;
    }

    if(CurrentCC != ExpectedCC)
    {
      m_Started = false;
      xBufferReset();
      m_LastContinuityCounter = CurrentCC;

      return eResult::StreamPacketLost;
    }
  }

  m_LastContinuityCounter = CurrentCC;

  //===========================================================================================================================================================================
  // Payload offset
  //
  // TS packet:
  // [4 B TS header][opcjonalne AF][payload]
  //
  // Jeśli AF występuje, jego rozmiar bierzemy z AdaptationField->getNumBytes().
  //===========================================================================================================================================================================

  uint32_t PayloadOffset = xTS::TS_HeaderLength;

  if(PacketHeader->hasAdaptationField())
  {
    if(AdaptationField == nullptr)
    {
      return eResult::StreamPacketLost;
    }

    PayloadOffset += AdaptationField->getNumBytes();
  }

  if(PayloadOffset >= xTS::TS_PacketLength)
  {
    return eResult::StreamPacketLost;
  }

  uint32_t PayloadSize = xTS::TS_PacketLength - PayloadOffset;

  //===========================================================================================================================================================================
  // Początek nowego PES
  //
  // Dla pakietów TS niosących PES:
  // payload_unit_start_indicator == 1 oznacza, że payload TS zaczyna się od pierwszego bajtu PES packet.
  //===========================================================================================================================================================================

  if(PacketHeader->getPayloadUnitStartIndicator() == 1)
  {
    xBufferReset();
    m_Started = true;

    if(PayloadSize < xTS::PES_HeaderLength)
    {
      m_Started = false;
      return eResult::StreamPacketLost;
    }

    const int32_t HeaderLength = m_PESH.Parse(TransportStreamPacket + PayloadOffset);

    if(HeaderLength == NOT_VALID)
    {
      m_Started = false;
      return eResult::StreamPacketLost;
    }

    const uint32_t ExpectedPESSize = m_PESH.getFullPacketLength();

    // Jeśli PES_packet_length != 0, znamy dokładny rozmiar PES.
    // Nie dopisujemy bajtów poza koniec tego PES.
    if(ExpectedPESSize != 0 && PayloadSize > ExpectedPESSize)
    {
      PayloadSize = ExpectedPESSize;
    }

    xBufferAppend(TransportStreamPacket + PayloadOffset, int32_t(PayloadSize));

    if(ExpectedPESSize != 0 && m_DataOffset >= ExpectedPESSize)
    {
      m_Started = false;
      return eResult::AssemblingFinished;
    }

    return eResult::AssemblingStarted;
  }

  //===========================================================================================================================================================================
  // Kontynuacja aktualnego PES
  //===========================================================================================================================================================================

  if(!m_Started)
  {
    return eResult::StreamPacketLost;
  }

  const uint32_t ExpectedPESSize = m_PESH.getFullPacketLength();

  if(ExpectedPESSize != 0)
  {
    if(m_DataOffset >= ExpectedPESSize)
    {
      m_Started = false;
      return eResult::AssemblingFinished;
    }

    const uint32_t BytesLeft = ExpectedPESSize - m_DataOffset;

    if(PayloadSize > BytesLeft)
    {
      PayloadSize = BytesLeft;
    }
  }

  xBufferAppend(TransportStreamPacket + PayloadOffset, int32_t(PayloadSize));

  if(ExpectedPESSize != 0 && m_DataOffset >= ExpectedPESSize)
  {
    m_Started = false;
    return eResult::AssemblingFinished;
  }

  return eResult::AssemblingContinue;
}

//=============================================================================================================================================================================

void xPES_Assembler::PrintResult(eResult Result) const
{
  switch(Result)
  {
    case eResult::UnexpectedPID:
    {
      break;
    }

    case eResult::StreamPacketLost:
    {
      printf(" PcktLost");
      break;
    }

    case eResult::AssemblingStarted:
    {
      printf(" Started ");
      PrintPESH();
      break;
    }

    case eResult::AssemblingContinue:
    {
      printf(" Continue");
      break;
    }

    case eResult::AssemblingFinished:
    {
      printf(" Finished PES: Len=%d DataLen=%u",
             getNumPacketBytes(),
             getPacketDataLength());
      break;
    }

    default:
    {
      break;
    }
  }
}

//=============================================================================================================================================================================