#include "tsTransportStreamHeader.h"
#include "tsTS.h"
#include <iostream>

//=============================================================================================================================================================================
// xTS_PacketHeader
//=============================================================================================================================================================================

/// @brief Reset - reset all TS packet header fields
void xTS_PacketHeader::Reset()
{
  m_SB  = 0;
  m_E   = 0;
  m_S   = 0;
  m_T   = 0;
  m_PID = 0;
  m_TSC = 0;
  m_AFC = 0;
  m_CC  = 0;
}

/**
  @brief Parse all TS packet header fields
  @param Input is pointer to buffer containing TS packet
  @return Number of parsed bytes (4 on success, -1 on failure) 
 */
int32_t xTS_PacketHeader::Parse(const uint8_t* Input) 
{
  const uint32_t* HeaderPtr = (uint32_t*)Input;
  const uint32_t Header = xSwapBytes32(*HeaderPtr);
  
  // | SB (8) | E (1) | S (1) | T (1) | PID (13) | TSC (2) | AFC (2) | CC (4) | // 32 bits

  m_SB  =  uint8_t((Header & 0b11111111000000000000000000000000) >> 24); // (SB ) :  8 bits
  m_E   =  uint8_t((Header & 0b00000000100000000000000000000000) >> 23); // (E  ) :  1 bit
  m_S   =  uint8_t((Header & 0b00000000010000000000000000000000) >> 22); // (S  ) :  1 bit
  m_T   =  uint8_t((Header & 0b00000000001000000000000000000000) >> 21); // (T  ) :  1 bit
  m_PID = uint16_t((Header & 0b00000000000111111111111100000000) >> 8);  // (PID) : 13 bits
  m_TSC =  uint8_t((Header & 0b00000000000000000000000011000000) >> 6);  // (TSC) :  2 bits
  m_AFC =  uint8_t((Header & 0b00000000000000000000000000110000) >> 4);  // (AFC) :  2 bits
  m_CC  =  uint8_t((Header & 0b00000000000000000000000000001111));       // (CC ) :  4 bits
  
  if (m_SB != 0x47) return NOT_VALID;
  return xTS::TS_HeaderLength;
}

/// @brief Print all TS packet header fields
void xTS_PacketHeader::Print() const
{
  printf("SB=%3d E=%d S=%d P=%d PID=%4d TSC=%d AF=%d CC=%2d",
         (int)m_SB,
         (int)m_E,
         (int)m_S,
         (int)m_T,
         (int)m_PID,
         (int)m_TSC,
         (int)m_AFC,
         (int)m_CC);
}

