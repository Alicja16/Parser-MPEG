#include "tsMPG123Decoder.h"

#include <cstdio>

//=============================================================================================================================================================================
// xMPG123Decoder
//=============================================================================================================================================================================

xMPG123Decoder::xMPG123Decoder()
{
  m_Handle = nullptr;
  m_MPG123Initialized = false;
  m_Started = false;

  m_OutputFileName.clear();

  m_OutputBuffer.resize(16384);

  m_SampleRate = 0;
  m_NumChannels = 0;
  m_Encoding = 0;
}

//=============================================================================================================================================================================

xMPG123Decoder::~xMPG123Decoder()
{
  Close();
}

//=============================================================================================================================================================================

bool xMPG123Decoder::Init(const char* OutputFileName)
{
  Close();

  if(OutputFileName == nullptr)
  {
    return false;
  }

  m_OutputFileName = OutputFileName;

  if(mpg123_init() != MPG123_OK)
  {
    printf("ERROR: mpg123_init failed\n");
    return false;
  }

  m_MPG123Initialized = true;

  int ErrorCode = MPG123_OK;

  m_Handle = mpg123_new(nullptr, &ErrorCode);

  if(m_Handle == nullptr)
  {
    printf("ERROR: mpg123_new failed, code=%d\n", ErrorCode);
    Close();
    return false;
  }

  if(!xSetupFormats())
  {
    printf("ERROR: mpg123 format setup failed\n");
    Close();
    return false;
  }

  if(mpg123_open_feed(m_Handle) != MPG123_OK)
  {
    printf("ERROR: mpg123_open_feed failed: %s\n", mpg123_strerror(m_Handle));
    Close();
    return false;
  }

  m_Started = true;

  return true;
}

//=============================================================================================================================================================================

bool xMPG123Decoder::xSetupFormats()
{
  if(m_Handle == nullptr)
  {
    return false;
  }

  // We force signed 16-bit PCM output, because it is simple and valid for WAVE PCM.
  if(mpg123_format_none(m_Handle) != MPG123_OK)
  {
    return false;
  }

  const long* Rates = nullptr;
  size_t NumRates = 0;

  mpg123_rates(&Rates, &NumRates);

  for(size_t i = 0; i < NumRates; i++)
  {
    mpg123_format(
      m_Handle,
      Rates[i],
      MPG123_MONO | MPG123_STEREO,
      MPG123_ENC_SIGNED_16
    );
  }

  return true;
}

//=============================================================================================================================================================================

bool xMPG123Decoder::Decode(const uint8_t* Data, uint32_t NumBytes)
{
  if(!m_Started || m_Handle == nullptr)
  {
    return false;
  }

  if(Data == nullptr || NumBytes == 0)
  {
    return true;
  }

  const unsigned char* InputData = Data;
  size_t InputSize = NumBytes;

  while(true)
  {
    size_t Done = 0;

    const int Result = mpg123_decode(
      m_Handle,
      InputData,
      InputSize,
      m_OutputBuffer.data(),
      m_OutputBuffer.size(),
      &Done
    );

    // After first call, data is fed. Next calls only drain internal decoder buffers.
    InputData = nullptr;
    InputSize = 0;

    if(Result == MPG123_NEW_FORMAT)
    {
      if(!xHandleNewFormat())
      {
        return false;
      }

      if(Done > 0)
      {
        if(!xWriteDecodedBytes(m_OutputBuffer.data(), Done))
        {
          return false;
        }
      }

      continue;
    }

    if(Done > 0)
    {
      if(!xWriteDecodedBytes(m_OutputBuffer.data(), Done))
      {
        return false;
      }
    }

    if(Result == MPG123_OK)
    {
      // More decoded data may be available, continue draining.
      continue;
    }

    if(Result == MPG123_NEED_MORE)
    {
      // Decoder needs next compressed packet.
      break;
    }

    printf("ERROR: mpg123_decode failed: %s\n", mpg123_strerror(m_Handle));
    return false;
  }

  return true;
}

//=============================================================================================================================================================================

bool xMPG123Decoder::xHandleNewFormat()
{
  if(m_Handle == nullptr)
  {
    return false;
  }

  long Rate = 0;
  int Channels = 0;
  int Encoding = 0;

  if(mpg123_getformat(m_Handle, &Rate, &Channels, &Encoding) != MPG123_OK)
  {
    printf("ERROR: mpg123_getformat failed: %s\n", mpg123_strerror(m_Handle));
    return false;
  }

  if(Encoding != MPG123_ENC_SIGNED_16)
  {
    printf("ERROR: unsupported mpg123 output encoding: %d\n", Encoding);
    return false;
  }

  if(Rate <= 0 || Channels <= 0)
  {
    printf("ERROR: invalid mpg123 audio format\n");
    return false;
  }

  if(!m_WaveWriter.IsOpen())
  {
    if(!m_WaveWriter.Open(m_OutputFileName.c_str(), uint16_t(Channels), uint32_t(Rate), 16))
    {
      printf("ERROR: cannot open WAVE output file: %s\n", m_OutputFileName.c_str());
      return false;
    }

    printf("MPG123: new format: %ld Hz, %d channels, signed 16-bit PCM\n", Rate, Channels);
  }
  else
  {
    // In this project we assume that the audio format does not change mid-stream.
    if(Rate != m_SampleRate || Channels != m_NumChannels || Encoding != m_Encoding)
    {
      printf("ERROR: audio format changed during decoding\n");
      return false;
    }
  }

  m_SampleRate = Rate;
  m_NumChannels = Channels;
  m_Encoding = Encoding;

  return true;
}

//=============================================================================================================================================================================

bool xMPG123Decoder::xWriteDecodedBytes(const uint8_t* Data, size_t NumBytes)
{
  if(Data == nullptr || NumBytes == 0)
  {
    return true;
  }

  if(!m_WaveWriter.IsOpen())
  {
    // Format should normally be known before decoded PCM is produced.
    if(!xHandleNewFormat())
    {
      return false;
    }
  }

  return m_WaveWriter.Write(Data, uint32_t(NumBytes));
}

//=============================================================================================================================================================================

void xMPG123Decoder::Close()
{
  m_WaveWriter.Close();

  if(m_Handle != nullptr)
  {
    mpg123_close(m_Handle);
    mpg123_delete(m_Handle);
    m_Handle = nullptr;
  }

  if(m_MPG123Initialized)
  {
    mpg123_exit();
    m_MPG123Initialized = false;
  }

  m_Started = false;

  m_SampleRate = 0;
  m_NumChannels = 0;
  m_Encoding = 0;
}

//=============================================================================================================================================================================