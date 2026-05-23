#pragma once
#include "tsCommon.h"
#include "tsTS.h"
#include <string>

//=============================================================================================================================================================================
// MPEG-TS Adaptation field:`
// `
// Adaptation field length               (AFL) : 8 bits
// Discontinuity indicator               (DC ) : 1 bit
// Random access indicator               (RA ) : 1 bit
// Elementary stream priority indicator  (SP ) : 1 bit
// Program Clock Reference flag          (PR ) : 1 bit
// Original Program Clock Reference flag (OR ) : 1 bit
// Splicing point flag                   (SF ) : 1 bit
// Transport private data flag           (TP ) : 1 bit
// Adaptation field extension flag       (EX ) : 1 bit

// Adaptation Field
//  ├── flags
//  ├── optional fields
//  │    ├── PCR
//  │    ├── OPCR
//  │    ├── splice_countdown
//  │    ├── transport_private_data
//  │    └── extension
//  │         ├── flags
//  │         ├── LTW
//  │         ├── piecewise_rate
//  │         └── seamless_splice
//  └── stuffing

class xTS_AdaptationField
{
protected:
  // setup / derived values
  uint8_t  m_AdaptationFieldControl;
  uint32_t m_NumBytes;
  uint32_t m_StuffingBytes;

  // mandatory fields
  uint8_t  m_AdaptationFieldLength;
  uint8_t  m_DC;
  uint8_t  m_RA;
  uint8_t  m_SP;
  uint8_t  m_PR;
  uint8_t  m_OR;
  uint8_t  m_SF;
  uint8_t  m_TP;
  uint8_t  m_EX;

  // optional fields parsed in this stage
  uint64_t m_PCR;
  uint64_t m_OPCR;

public:
  void    Reset();
  int32_t Parse(const uint8_t* PacketBuffer, uint8_t AdaptationFieldControl);
  void    Print() const;

public:
  // mandatory fields
  uint8_t getAdaptationFieldLength() const { return m_AdaptationFieldLength; }
  uint8_t getDC() const { return m_DC; }
  uint8_t getRA() const { return m_RA; }
  uint8_t getSP() const { return m_SP; }
  uint8_t getPR() const { return m_PR; }
  uint8_t getOR() const { return m_OR; }
  uint8_t getSF() const { return m_SF; }
  uint8_t getTP() const { return m_TP; }
  uint8_t getEX() const { return m_EX; }

  // optional fields
  bool     hasPCR()  const { return m_PR != 0; }
  bool     hasOPCR() const { return m_OR != 0; }
  uint64_t getPCR()  const { return m_PCR; }
  uint64_t getOPCR() const { return m_OPCR; }

  // derived values
  uint32_t getNumBytes() const { return m_NumBytes; }
  uint32_t getStuffingBytes() const { return m_StuffingBytes; }
};