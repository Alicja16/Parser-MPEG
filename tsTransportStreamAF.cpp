//=============================================================================================================================================================================
#include "tsTransportStreamAF.h"
#include "tsTS.h"
#include <iostream>
//=============================================================================================================================================================================
// xTS_AdaptationField
//=============================================================================================================================================================================

/// @brief Reset - reset all TS packet AdaptationField
void xTS_AdaptationField::Reset()
{
  //setup
  m_AdaptationFieldControl = 0;
  m_NumBytes = 0;
  m_StuffingBytes = 0;

  //mandatory fields
  m_AdaptationFieldLength = 0;
  m_DC = 0;
  m_RA = 0;
  m_SP = 0;
  m_PR = 0;
  m_OR = 0;
  m_SF = 0;
  m_TP = 0;
  m_EX = 0;

  //optional fields
  // PCR
  m_PCR = 0;
  // OPCR
  m_OPCR = 0;
  // SpliceCountdown
  m_SpliceCountdown = 0;
  // TransportPrivate
  m_TransportPrivateDataLength = 0;
  m_TransportPrivateDataPtr = nullptr;

  // Extension
  m_AdaptationFieldExtensionLength = 0;

  // Extension flags
  m_LTW_flag = 0;
  m_PiecewiseRate_flag = 0;
  m_SeamlessSplice_flag = 0;

  // Legal Time Window
  m_LTW_valid_flag = 0;
  m_LTW_offset = 0;

  // Piecewise Rate
  m_PiecewiseRate = 0;

  // Seamless Splice
  m_SpliceType = 0;
  m_DTSNextAccessUnit = 0;
}

/**
  @brief Parse adaptation field
  @param PacketBuffer is pointer to buffer containing TS packet
  @param AdaptationFieldControl is value of Adaptation Field Control field of
  corresponding TS packet header
  @return Number of parsed bytes (length of AF or -1 on failure)
*/

  
static uint64_t ReadPCR_OPCR(const uint8_t* Data)
{
  // byte0: PCR_base[32..25]
  // byte1: PCR_base[24..17]
  // byte2: PCR_base[16..9]
  // byte3: PCR_base[8..1]
  // byte4: PCR_base[0] | reserved(6) | PCR_ext[8]
  // byte5: PCR_ext[7..0]

  // PCR_base (33 bits)
  // [32][31][30][29][28][27][26][25][24][23][22][21][20][19][18][17][16][15][14][13][12][11][10][ 9][ 8][ 7][ 6][ 5][ 4][ 3][ 2][ 1][ 0]
  //   1   1   1   1   1   1   1   1
  //                                   1   1   1   1   1   1   1   1
  //                                                                   1   1   1   1   1   1   1   1
  //                                                                                                   1   1   1   1   1   1   1   1
  //                                                                                                                                   1   0   0   0   0   0   0   0
  uint64_t PCR_base =
    (uint64_t(Data[0]) << 25) |
    (uint64_t(Data[1]) << 17) |
    (uint64_t(Data[2]) <<  9) |
    (uint64_t(Data[3]) <<  1) |
    (uint64_t(Data[4] & 0b10000000) >> 7);
  
  // PCR_ext (9 bits)
  // [ 8][ 7][ 6][ 5][ 4][ 3][ 2][ 1][ 0]
  //   1
  //       1   1   1   1   1   1   1   1
  uint16_t PCR_ext =
    (uint16_t(Data[4] & 0b00000001) << 8) |
    (uint16_t(Data[5]));
  
  // PCR_base 90 kHz
  // PCR_ext  27 MHz

  // 1 PCR_base = 300 ticks PCR
  // 27 MHz / 90 kHz = 300
  
  return PCR_base*300 + PCR_ext;
}

int32_t xTS_AdaptationField::Parse(const uint8_t* PacketBuffer, uint8_t AdaptationFieldControl)
{
  // AF = [AFL][flags][optional fields][stuffing]
  m_AdaptationFieldControl = AdaptationFieldControl;
  // ======================== Is there AdaptationField ========================
  if(AdaptationFieldControl != 2 && AdaptationFieldControl != 3)
  {
    m_NumBytes = 0;
    m_StuffingBytes = 0;
    return 0;
  }
  // ==========================================================================
  
  uint32_t offset = xTS::TS_HeaderLength; // offset after header - 4 byte (where are we)
  m_AdaptationFieldLength = PacketBuffer[offset];
  offset += 1;

  if (m_AdaptationFieldLength == 0)
  {
    m_NumBytes = 1;
    m_StuffingBytes = 0;
    return m_NumBytes;
  }

  uint8_t Flags = PacketBuffer[offset];
  
  m_DC = uint8_t((Flags & 0b10000000) >> 7);
  m_RA = uint8_t((Flags & 0b01000000) >> 6);
  m_SP = uint8_t((Flags & 0b00100000) >> 5);
  m_PR = uint8_t((Flags & 0b00010000) >> 4);
  m_OR = uint8_t((Flags & 0b00001000) >> 3);
  m_SF = uint8_t((Flags & 0b00000100) >> 2);
  m_TP = uint8_t((Flags & 0b00000010) >> 1);
  m_EX = uint8_t((Flags & 0b00000001));

  uint32_t BytesLeft = m_AdaptationFieldLength - 1;
  offset += 1;


  if(m_PR){
    if(BytesLeft < 6) return NOT_VALID;
    m_PCR = ReadPCR_OPCR(&PacketBuffer[offset]);
    offset += 6; // ← 6 bytes of PCR
    BytesLeft -= 6;
  }

  if(m_OR){
    if(BytesLeft < 6) return NOT_VALID;
    m_OPCR = ReadPCR_OPCR(&PacketBuffer[offset]);
    offset += 6; // ← 6 bytes of OPCR
    BytesLeft -= 6;
  }

  if(m_SF){
    if(BytesLeft < 1) return NOT_VALID;
    m_SpliceCountdown = int8_t(PacketBuffer[offset]);
    offset += 1; // ← 1 byte of splice countdown
    BytesLeft -= 1;
  }

  if(m_TP){
    if(BytesLeft < 1) return NOT_VALID;
    m_TransportPrivateDataLength = PacketBuffer[offset];
    offset += 1;
    BytesLeft -= 1;

    if(BytesLeft < m_TransportPrivateDataLength) return NOT_VALID;
    m_TransportPrivateDataPtr = &PacketBuffer[offset];

    offset += m_TransportPrivateDataLength;
    BytesLeft -= m_TransportPrivateDataLength;
  }

  if(m_EX){
    if(BytesLeft < 1) return NOT_VALID;

    m_AdaptationFieldExtensionLength = PacketBuffer[offset]; // ← 1 byte of data length
    offset += 1;
    BytesLeft -= 1;

    if(BytesLeft < m_AdaptationFieldExtensionLength) return NOT_VALID;

    uint32_t ExtensionBytesLeft = m_AdaptationFieldExtensionLength;
    if(ExtensionBytesLeft < 1) return NOT_VALID;

    uint8_t ExtFlags = PacketBuffer[offset];
    offset += 1;
    BytesLeft -= 1;
    ExtensionBytesLeft -= 1;
    
    m_LTW_flag           = uint8_t((ExtFlags & 0b10000000) >> 7);
    m_PiecewiseRate_flag = uint8_t((ExtFlags & 0b01000000) >> 6);
    m_SeamlessSplice_flag= uint8_t((ExtFlags & 0b00100000) >> 5);

    // ---------------- LTW ----------------
    // ltw_valid_flag  1 bit
    // ltw_offset     15 bits
    //                ---------
    //                 2 bytes
    if(m_LTW_flag)
    {
      if(ExtensionBytesLeft < 2) return NOT_VALID;
      m_LTW_valid_flag = uint8_t((PacketBuffer[offset] & 0b10000000) >> 7);

      m_LTW_offset = (uint16_t(PacketBuffer[offset] & 0b01111111) << 8) | uint16_t(PacketBuffer[offset+1]);

      offset += 2;
      BytesLeft -= 2;
      ExtensionBytesLeft -= 2;
    }

    // ---------------- piecewise_rate ----------------
    // reserved        2 bits
    // piecewise_rate 22 bits
    //                ---------
    //                 3 bytes
    if(m_PiecewiseRate_flag)
    {
      if(ExtensionBytesLeft < 3) return NOT_VALID;

      m_PiecewiseRate = 
      (uint32_t(PacketBuffer[offset] & 0b00111111) << 16) |
      (uint32_t(PacketBuffer[offset+1]) << 8) |
      uint32_t(PacketBuffer[offset+2]);

      offset += 3;
      BytesLeft -= 3;
      ExtensionBytesLeft -= 3;
    }

    // ---------------- seamless_splice ----------------
    // splice_type           4 bits
    // DTS_next_AU[32..30]   3 bits
    // marker_bit            1 bit
    // DTS_next_AU[29..15]   15 bits
    // marker_bit            1 bit
    // DTS_next_AU[14..0]    15 bits
    // marker_bit            1 bit
    //                       ---------
    //                       5 bytes
    if(m_SeamlessSplice_flag)
    {
      if(ExtensionBytesLeft < 5) return NOT_VALID;

      m_SpliceType = uint8_t((PacketBuffer[offset] & 0b11110000) >> 4);

      // byte0 = [splice_type(4)] [DTS 32..30 (3)] [marker(1)]
      // byte1 = [DTS 29..22 (8)]
      // byte2 = [DTS 21..15 (7)] [marker(1)]
      // byte3 = [DTS 14..7  (8)]
      // byte4 = [DTS 6..0   (7)] [marker(1)]

      // DTS (33 bits)
      // [32][31][30][29][28][27][26][25][24][23][22][21][20][19][18][17][16][15][14][13][12][11][10][ 9][ 8][ 7][ 6][ 5][ 4][ 3][ 2][ 1][ 0]
      //  1   1    1   0  
      //               1   1   1   1   1   1   1   1
      //                                               1   1   1   1   1   1   1   0
      //                                                                           1   1   1   1   1   1   1   1
      //                                                                                                           1   1   1   1   1   1   1   0  0
      m_DTSNextAccessUnit =
      (uint64_t(PacketBuffer[offset] & 0b00001110)   << 29) |
      (uint64_t(PacketBuffer[offset+1])              << 22) | //29 - 22
      (uint64_t(PacketBuffer[offset+2] & 0b11111110) << 14) | //21 - 15
      (uint64_t(PacketBuffer[offset+3])              <<  7) | //14 -  7
      (uint64_t(PacketBuffer[offset+4] & 0b11111100) >>  2);  // 6 -  0

      offset += 5;
      BytesLeft -= 5;
      ExtensionBytesLeft -= 5;
    }

    // reserved
    offset += ExtensionBytesLeft;
    BytesLeft -= ExtensionBytesLeft;
    ExtensionBytesLeft = 0;
  }
  m_StuffingBytes = BytesLeft;
  m_NumBytes = 1 + m_AdaptationFieldLength;

  return m_NumBytes; // ← how many bytes does the entire AF have
}

/// @brief Print all TS packet header fields
void xTS_AdaptationField::Print() const
{
  printf(" AF: L=%3d DC=%d RA=%d SP=%d PR=%d OR=%d SF=%d TP=%d EX=%d Stuffing=%d",
         (int)m_AdaptationFieldLength,
         (int)m_DC,
         (int)m_RA,
         (int)m_SP,
         (int)m_PR,
         (int)m_OR,
         (int)m_SF,
         (int)m_TP,
         (int)m_EX,
         (int)m_StuffingBytes);

  if(m_PR)
  {
    double PCRtime = double(m_PCR) / double(xTS::ExtendedClockFrequency_Hz);
    printf(" PCR=%" PRIu64 " (Time=%0.6fs)", m_PCR, PCRtime);
  }

  if(m_OR)
  {
    double OPCRtime = double(m_OPCR) / double(xTS::ExtendedClockFrequency_Hz);
    printf(" OPCR=%" PRIu64 " (Time=%0.6fs)", m_OPCR, OPCRtime);
  }

  if(m_SF)
  {
    printf(" SC=%d", (int)m_SpliceCountdown);
  }

  if(m_TP)
  {
    printf(" TPDL=%d", (int)m_TransportPrivateDataLength);
  }

  if(m_EX)
  {
    printf(" EXL=%d", (int)m_AdaptationFieldExtensionLength);
    printf(" LTW=%d PWR=%d SS=%d",
           (int)m_LTW_flag,
           (int)m_PiecewiseRate_flag,
           (int)m_SeamlessSplice_flag);

    if(m_LTW_flag)
    {
      printf(" LTW_valid=%d LTW_offset=%d",
             (int)m_LTW_valid_flag,
             (int)m_LTW_offset);
    }

    if(m_PiecewiseRate_flag)
    {
      printf(" PiecewiseRate=%u", (unsigned)m_PiecewiseRate);
    }

    if(m_SeamlessSplice_flag)
    {
      printf(" SpliceType=%d DTS_next_AU=%" PRIu64,
             (int)m_SpliceType,
             m_DTSNextAccessUnit);
    }
  }
}

//=============================================================================================================================================================================
