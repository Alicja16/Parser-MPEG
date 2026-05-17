#pragma once
#include "tsCommon.h"
#include "tsTS.h"
#include <string>


class xTS_PacketHeader
{
public:
  enum class ePID : uint16_t
  {
    PAT  = 0x0000,
    CAT  = 0x0001,
    TSDT = 0x0002,
    IPMT = 0x0003,
    NIT  = 0x0010, //DVB specific PID
    SDT  = 0x0011, //DVB specific PID
    NuLL = 0x1FFF,
  };

protected:
  //============header fields======================================:
  uint8_t  m_SB;
  uint8_t  m_E;
  uint8_t  m_S;
  uint8_t  m_T;
  uint16_t m_PID;
  uint8_t  m_TSC;
  uint8_t  m_AFC;
  uint8_t  m_CC;
  //================================================================

public:
  void     Reset();
  int32_t  Parse(const uint8_t* Input);
  void     Print() const;

public:
  //===========direct acces to header field values====================:
  // Sync byte                    (SB ) :  8 bits
  uint8_t  getSyncByte() const { return m_SB; }
  // Transport error indicator    (E  ) :  1 bit
  uint8_t  getTransportErrorIndicator() const { return m_E; }
  // Payload unit start indicator (S  ) :  1 bit
  uint8_t  getPayloadUnitStartIndicator() const { return m_S; }
  // Transport priority           (T  ) :  1 bit
  uint8_t  getTransportPriority() const { return m_T; }
  // Packet Identifier            (PID) : 13 bits
  uint16_t  getPacketIdentifier() const { return m_PID; }
  // Transport scrambling control (TSC) :  2 bits
  uint8_t  getTransportScramblingControl() const { return m_TSC; }
  // Adaptation field control     (AFC) :  2 bits
  uint8_t  getAdaptationFieldControl() const { return m_AFC; }
  // Continuity counter           (CC ) :  4 bits
  uint8_t  getContinuityCounter() const { return m_CC; }
  //==================================================================

public:
  //====================derrived informations=========================
  // m_AFC  -  Adaptation Field Control (2 bity)
  // 00 = reserved (not used)
  // 01 = payload only
  // 10 = adaptation field only
  // 11 = adaptation field + payload

  bool     hasAdaptationField() const { return (m_AFC == 2 || m_AFC == 3); }
  bool     hasPayload        () const { return (m_AFC == 1 || m_AFC == 3); }
  //==================================================================
};


