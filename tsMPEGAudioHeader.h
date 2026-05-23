#pragma once

#include "tsCommon.h"
#include "tsTS.h"

#include <cstdint>

//=============================================================================================================================================================================
// MPEG Audio Frame Header
//
// Scope:
// - find and parse MPEG Audio Frame Header from elementary audio stream
// - decode:
//   syncword
//   MPEG audio version
//   layer
//   protection bit
//   bitrate
//   sampling rate
//   padding
//   channel mode
//
// This is an additional task. It does not decode audio.
//=============================================================================================================================================================================

class xMPEGAudioHeader
{
protected:

  // raw 32-bit header
  uint32_t m_Header;

  // fields
  uint16_t m_Syncword;          // 11 bits
  uint8_t  m_VersionID;         // 2 bits
  uint8_t  m_LayerID;           // 2 bits
  uint8_t  m_ProtectionBit;     // 1 bit
  uint8_t  m_BitrateIndex;      // 4 bits
  uint8_t  m_SamplingRateIndex; // 2 bits
  uint8_t  m_PaddingBit;        // 1 bit
  uint8_t  m_PrivateBit;        // 1 bit
  uint8_t  m_ChannelMode;       // 2 bits
  uint8_t  m_ModeExtension;     // 2 bits
  uint8_t  m_Copyright;         // 1 bit
  uint8_t  m_Original;          // 1 bit
  uint8_t  m_Emphasis;          // 2 bits

  // derived values
  int32_t m_BitrateKbps;
  int32_t m_SamplingRateHz;
  int32_t m_FrameLengthBytes;

public:

  void Reset();
  int32_t Parse(const uint8_t* Data);
  void Print() const;

public:

  bool isValid() const;

  uint32_t getHeader() const { return m_Header; }

  uint16_t getSyncword() const { return m_Syncword; }
  uint8_t  getVersionID() const { return m_VersionID; }
  uint8_t  getLayerID() const { return m_LayerID; }
  uint8_t  getProtectionBit() const { return m_ProtectionBit; }
  uint8_t  getBitrateIndex() const { return m_BitrateIndex; }
  uint8_t  getSamplingRateIndex() const { return m_SamplingRateIndex; }
  uint8_t  getPaddingBit() const { return m_PaddingBit; }
  uint8_t  getChannelMode() const { return m_ChannelMode; }

  int32_t getBitrateKbps() const { return m_BitrateKbps; }
  int32_t getSamplingRateHz() const { return m_SamplingRateHz; }
  int32_t getFrameLengthBytes() const { return m_FrameLengthBytes; }

protected:

  const char* xGetVersionName() const;
  const char* xGetLayerName() const;
  const char* xGetChannelModeName() const;

  int32_t xGetBitrateKbps() const;
  int32_t xGetSamplingRateHz() const;
  int32_t xCalculateFrameLengthBytes() const;
};

//=============================================================================================================================================================================