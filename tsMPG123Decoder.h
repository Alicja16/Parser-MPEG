#pragma once

#include "tsCommon.h"
#include "tsWaveWriter.h"

#include <cstdint>
#include <string>
#include <vector>
#include <mpg123.h>

//=============================================================================================================================================================================
// MPG123 Decoder
//
// - initialize mpg123 decoder
// - accept MPEG audio bytes packet by packet
// - decode compressed MPEG Audio to PCM samples
// - write decoded PCM samples to WAVE file through xWaveWriter
//
// This is an additional task:
// "decode audio stream using mpg123 packet by packet, without saving intermediate results to disk"
//=============================================================================================================================================================================

class xMPG123Decoder
{
protected:

  mpg123_handle* m_Handle;
  bool           m_MPG123Initialized;
  bool           m_Started;

  std::string    m_OutputFileName;
  xWaveWriter    m_WaveWriter;

  std::vector<uint8_t> m_OutputBuffer;

  long    m_SampleRate;
  int32_t m_NumChannels;
  int32_t m_Encoding;

public:

  xMPG123Decoder();
  ~xMPG123Decoder();

  bool Init(const char* OutputFileName);
  bool Decode(const uint8_t* Data, uint32_t NumBytes);
  void Close();

protected:

  bool xSetupFormats();
  bool xHandleNewFormat();
  bool xWriteDecodedBytes(const uint8_t* Data, size_t NumBytes);
};

//=============================================================================================================================================================================