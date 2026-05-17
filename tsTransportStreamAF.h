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
  //setup
  uint8_t  m_AdaptationFieldControl;
  uint32_t m_NumBytes;
  uint32_t m_StuffingBytes;

  //mandatory fields
  uint8_t  m_AdaptationFieldLength;
  uint8_t  m_DC;
  uint8_t  m_RA;
  uint8_t  m_SP;
  uint8_t  m_PR;
  uint8_t  m_OR;
  uint8_t  m_SF;
  uint8_t  m_TP;
  uint8_t  m_EX;

  //optional fields
  // PCR =====================================================
  // synchronizacja poprawna audio i video
  // przeliczać PCR na sekundy,
  // mierzyć odstępy PCR,
  // sprawdzać regularność,
  // analizować timing strumienia
  uint64_t  m_PCR;

  // OPCR=====================================================
  // porównywać PCR i OPCR,
  // wykrywać, że strumień był przetwarzany,
  // analizować przesunięcie po remuxie
  uint64_t  m_OPCR;

  // SpliceCountdown===========================================
  // kiedy nastąpi przełaczenie strumienia
  int8_t   m_SpliceCountdown;

  // TransportPrivate==========================================
  // producenta sprzętu,
  // operatora,
  // system warunkowego dostępu,
  // wewnętrzne metadane nadawcy
  uint8_t   m_TransportPrivateDataLength;
  const uint8_t* m_TransportPrivateDataPtr;

  // Extension=================================================
  uint8_t  m_AdaptationFieldExtensionLength;

  // Extension flags==================================
  uint8_t  m_LTW_flag;
  uint8_t  m_PiecewiseRate_flag;
  uint8_t  m_SeamlessSplice_flag;

  // Legal Time Window=======================
  // ograniczenia czasowe dostarczenia pakietów z punktu widzenia modelu buforów dekodera
  uint8_t  m_LTW_valid_flag;
  uint16_t m_LTW_offset;

  // Piecewise Rate==========================
  // opis modelu czasowego i przepływności widzianej przez system
  uint32_t m_PiecewiseRate;

  // Seamless Splice=========================
  // jak wykonać przejście między fragmentami strumienia bez zaburzenia czasu dekodowania
  uint8_t  m_SpliceType;
  uint64_t m_DTSNextAccessUnit;

public:
  void    Reset();
  int32_t Parse(const uint8_t* PacketBuffer, uint8_t AdaptationFieldControl);
  void    Print() const;

public:
  //mandatory fields
  uint8_t getAdaptationFieldLength () const { return m_AdaptationFieldLength ; }
  uint8_t getPR () const { return m_PR; }
  uint8_t getOR () const { return m_OR; }
  uint8_t getSF () const { return m_SF; }
  uint8_t getTP () const { return m_TP; }
  uint8_t getEX () const { return m_EX; }
  //derived values
  uint32_t getNumBytes () const { return m_NumBytes; }
  uint32_t getStuffingBytes () const { return m_StuffingBytes; }

};

//=============================================================================================================================================================================
