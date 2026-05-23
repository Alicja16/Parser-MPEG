#include "tsMPEGAudioHeader.h"

#include <cstdio>

//=============================================================================================================================================================================
// xMPEGAudioHeader
//=============================================================================================================================================================================

void xMPEGAudioHeader::Reset()
{
  m_Header = 0;

  m_Syncword = 0;
  m_VersionID = 0;
  m_LayerID = 0;
  m_ProtectionBit = 0;
  m_BitrateIndex = 0;
  m_SamplingRateIndex = 0;
  m_PaddingBit = 0;
  m_PrivateBit = 0;
  m_ChannelMode = 0;
  m_ModeExtension = 0;
  m_Copyright = 0;
  m_Original = 0;
  m_Emphasis = 0;

  m_BitrateKbps = 0;
  m_SamplingRateHz = 0;
  m_FrameLengthBytes = 0;
}

//=============================================================================================================================================================================

int32_t xMPEGAudioHeader::Parse(const uint8_t* Data)
{
  if(Data == nullptr)
  {
    return NOT_VALID;
  }

  Reset();

  // MPEG Audio Header is 32 bits, big-endian.
  m_Header =
      (uint32_t(Data[0]) << 24)
    | (uint32_t(Data[1]) << 16)
    | (uint32_t(Data[2]) <<  8)
    |  uint32_t(Data[3]);

  m_Syncword          = uint16_t((m_Header >> 21) & 0x7FF);
  m_VersionID         = uint8_t ((m_Header >> 19) & 0x03);
  m_LayerID           = uint8_t ((m_Header >> 17) & 0x03);
  m_ProtectionBit     = uint8_t ((m_Header >> 16) & 0x01);
  m_BitrateIndex      = uint8_t ((m_Header >> 12) & 0x0F);
  m_SamplingRateIndex = uint8_t ((m_Header >> 10) & 0x03);
  m_PaddingBit        = uint8_t ((m_Header >>  9) & 0x01);
  m_PrivateBit        = uint8_t ((m_Header >>  8) & 0x01);
  m_ChannelMode       = uint8_t ((m_Header >>  6) & 0x03);
  m_ModeExtension     = uint8_t ((m_Header >>  4) & 0x03);
  m_Copyright         = uint8_t ((m_Header >>  3) & 0x01);
  m_Original          = uint8_t ((m_Header >>  2) & 0x01);
  m_Emphasis          = uint8_t ( m_Header        & 0x03);

  if(!isValid())
  {
    return NOT_VALID;
  }

  m_BitrateKbps = xGetBitrateKbps();
  m_SamplingRateHz = xGetSamplingRateHz();
  m_FrameLengthBytes = xCalculateFrameLengthBytes();

  if(m_BitrateKbps <= 0 || m_SamplingRateHz <= 0 || m_FrameLengthBytes <= 0)
  {
    return NOT_VALID;
  }

  return 4;
}

//=============================================================================================================================================================================

bool xMPEGAudioHeader::isValid() const
{
  // Syncword must be 11 bits set to 1.
  if(m_Syncword != 0x7FF)
  {
    return false;
  }

  // VersionID == 01 is reserved.
  if(m_VersionID == 0b01)
  {
    return false;
  }

  // LayerID == 00 is reserved.
  if(m_LayerID == 0b00)
  {
    return false;
  }

  // Bitrate index 0000 means free format, 1111 is bad.
  // We do not support free format in this simple parser.
  if(m_BitrateIndex == 0x00 || m_BitrateIndex == 0x0F)
  {
    return false;
  }

  // Sampling rate index 11 is reserved.
  if(m_SamplingRateIndex == 0x03)
  {
    return false;
  }

  return true;
}

//=============================================================================================================================================================================

const char* xMPEGAudioHeader::xGetVersionName() const
{
  switch(m_VersionID)
  {
    case 0b00: return "MPEG 2.5";
    case 0b10: return "MPEG 2";
    case 0b11: return "MPEG 1";
    default:   return "reserved";
  }
}

//=============================================================================================================================================================================

const char* xMPEGAudioHeader::xGetLayerName() const
{
  switch(m_LayerID)
  {
    case 0b01: return "Layer III";
    case 0b10: return "Layer II";
    case 0b11: return "Layer I";
    default:   return "reserved";
  }
}

//=============================================================================================================================================================================

const char* xMPEGAudioHeader::xGetChannelModeName() const
{
  switch(m_ChannelMode)
  {
    case 0b00: return "Stereo";
    case 0b01: return "Joint stereo";
    case 0b10: return "Dual channel";
    case 0b11: return "Single channel";
    default:   return "unknown";
  }
}

//=============================================================================================================================================================================

int32_t xMPEGAudioHeader::xGetSamplingRateHz() const
{
  // Rows:
  // MPEG 1, MPEG 2, MPEG 2.5
  static const int32_t SamplingRateTable[3][3] =
  {
    { 44100, 48000, 32000 }, // MPEG 1
    { 22050, 24000, 16000 }, // MPEG 2
    { 11025, 12000,  8000 }  // MPEG 2.5
  };

  int32_t VersionRow = -1;

  if(m_VersionID == 0b11) { VersionRow = 0; } // MPEG 1
  if(m_VersionID == 0b10) { VersionRow = 1; } // MPEG 2
  if(m_VersionID == 0b00) { VersionRow = 2; } // MPEG 2.5

  if(VersionRow < 0 || m_SamplingRateIndex > 2)
  {
    return 0;
  }

  return SamplingRateTable[VersionRow][m_SamplingRateIndex];
}

//=============================================================================================================================================================================

int32_t xMPEGAudioHeader::xGetBitrateKbps() const
{
  // Table order:
  // index 0..15
  //
  // MPEG 1:
  // Layer I, Layer II, Layer III
  //
  // MPEG 2 / 2.5:
  // Layer I, Layer II, Layer III

  static const int32_t BitrateTable_MPEG1_L1[16] =
  {
    0, 32, 64, 96, 128, 160, 192, 224,
    256, 288, 320, 352, 384, 416, 448, 0
  };

  static const int32_t BitrateTable_MPEG1_L2[16] =
  {
    0, 32, 48, 56, 64, 80, 96, 112,
    128, 160, 192, 224, 256, 320, 384, 0
  };

  static const int32_t BitrateTable_MPEG1_L3[16] =
  {
    0, 32, 40, 48, 56, 64, 80, 96,
    112, 128, 160, 192, 224, 256, 320, 0
  };

  static const int32_t BitrateTable_MPEG2_L1[16] =
  {
    0, 32, 48, 56, 64, 80, 96, 112,
    128, 144, 160, 176, 192, 224, 256, 0
  };

  static const int32_t BitrateTable_MPEG2_L2L3[16] =
  {
    0, 8, 16, 24, 32, 40, 48, 56,
    64, 80, 96, 112, 128, 144, 160, 0
  };

  const bool IsMPEG1 = (m_VersionID == 0b11);

  if(IsMPEG1)
  {
    if(m_LayerID == 0b11) { return BitrateTable_MPEG1_L1[m_BitrateIndex]; }
    if(m_LayerID == 0b10) { return BitrateTable_MPEG1_L2[m_BitrateIndex]; }
    if(m_LayerID == 0b01) { return BitrateTable_MPEG1_L3[m_BitrateIndex]; }
  }
  else
  {
    if(m_LayerID == 0b11) { return BitrateTable_MPEG2_L1[m_BitrateIndex]; }
    if(m_LayerID == 0b10) { return BitrateTable_MPEG2_L2L3[m_BitrateIndex]; }
    if(m_LayerID == 0b01) { return BitrateTable_MPEG2_L2L3[m_BitrateIndex]; }
  }

  return 0;
}

//=============================================================================================================================================================================

int32_t xMPEGAudioHeader::xCalculateFrameLengthBytes() const
{
  if(m_BitrateKbps <= 0 || m_SamplingRateHz <= 0)
  {
    return 0;
  }

  const int32_t BitrateBps = m_BitrateKbps * 1000;

  // Layer I:
  // frame_length = (12 * bitrate / sampling_rate + padding) * 4
  if(m_LayerID == 0b11)
  {
    return ((12 * BitrateBps / m_SamplingRateHz) + m_PaddingBit) * 4;
  }

  // Layer II:
  // frame_length = 144 * bitrate / sampling_rate + padding
  if(m_LayerID == 0b10)
  {
    return (144 * BitrateBps / m_SamplingRateHz) + m_PaddingBit;
  }

  // Layer III:
  // MPEG 1:   144 * bitrate / sampling_rate + padding
  // MPEG 2/2.5: 72 * bitrate / sampling_rate + padding
  if(m_LayerID == 0b01)
  {
    if(m_VersionID == 0b11)
    {
      return (144 * BitrateBps / m_SamplingRateHz) + m_PaddingBit;
    }

    return (72 * BitrateBps / m_SamplingRateHz) + m_PaddingBit;
  }

  return 0;
}

//=============================================================================================================================================================================

void xMPEGAudioHeader::Print() const
{
  printf("MPEG Audio: Sync=0x%03X Version=%s Layer=%s Protection=%d Bitrate=%dkbps Fs=%dHz Padding=%d Channel=%s FrameLen=%d",
         unsigned(m_Syncword),
         xGetVersionName(),
         xGetLayerName(),
         int(m_ProtectionBit),
         int(m_BitrateKbps),
         int(m_SamplingRateHz),
         int(m_PaddingBit),
         xGetChannelModeName(),
         int(m_FrameLengthBytes));
}

//=============================================================================================================================================================================