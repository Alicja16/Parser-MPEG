#pragma once

#include "tsCommon.h"

#include <cstdint>
#include <cstdio>

//=============================================================================================================================================================================
// WAVE Writer
//
// Scope:
// - create PCM WAVE file
// - write WAVE header
// - append decoded PCM samples
// - update file sizes when closing
//
// This class does not decode audio.
// It only writes PCM data to .wav file.
//=============================================================================================================================================================================

class xWaveWriter
{
protected:

  FILE*    m_File;
  uint32_t m_DataBytesWritten;

  uint16_t m_NumChannels;
  uint32_t m_SampleRate;
  uint16_t m_BitsPerSample;

public:

  xWaveWriter();
  ~xWaveWriter();

  bool Open(const char* FileName, uint16_t NumChannels, uint32_t SampleRate, uint16_t BitsPerSample);
  bool Write(const uint8_t* Data, uint32_t NumBytes);
  void Close();

  bool IsOpen() const { return m_File != nullptr; }

protected:

  void xWriteHeader();
  void xUpdateHeader();
  void xWriteLE16(uint16_t Value);
  void xWriteLE32(uint32_t Value);
};

//=============================================================================================================================================================================