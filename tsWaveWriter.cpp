#include "tsWaveWriter.h"

//=============================================================================================================================================================================
// xWaveWriter
//=============================================================================================================================================================================

xWaveWriter::xWaveWriter()
{
  m_File = nullptr;
  m_DataBytesWritten = 0;

  m_NumChannels = 0;
  m_SampleRate = 0;
  m_BitsPerSample = 0;
}

//=============================================================================================================================================================================

xWaveWriter::~xWaveWriter()
{
  Close();
}

//=============================================================================================================================================================================

bool xWaveWriter::Open(const char* FileName, uint16_t NumChannels, uint32_t SampleRate, uint16_t BitsPerSample)
{
  Close();

  if(FileName == nullptr)
  {
    return false;
  }

  m_File = fopen(FileName, "wb");

  if(m_File == nullptr)
  {
    return false;
  }

  m_NumChannels = NumChannels;
  m_SampleRate = SampleRate;
  m_BitsPerSample = BitsPerSample;
  m_DataBytesWritten = 0;

  xWriteHeader();

  return true;
}

//=============================================================================================================================================================================

bool xWaveWriter::Write(const uint8_t* Data, uint32_t NumBytes)
{
  if(m_File == nullptr || Data == nullptr || NumBytes == 0)
  {
    return false;
  }

  const size_t NumWritten = fwrite(Data, 1, NumBytes, m_File);

  if(NumWritten != NumBytes)
  {
    return false;
  }

  m_DataBytesWritten += NumBytes;

  return true;
}

//=============================================================================================================================================================================

void xWaveWriter::Close()
{
  if(m_File != nullptr)
  {
    xUpdateHeader();
    fclose(m_File);
    m_File = nullptr;
  }
}

//=============================================================================================================================================================================

void xWaveWriter::xWriteLE16(uint16_t Value)
{
  fputc((Value >> 0) & 0xFF, m_File);
  fputc((Value >> 8) & 0xFF, m_File);
}

//=============================================================================================================================================================================

void xWaveWriter::xWriteLE32(uint32_t Value)
{
  fputc((Value >>  0) & 0xFF, m_File);
  fputc((Value >>  8) & 0xFF, m_File);
  fputc((Value >> 16) & 0xFF, m_File);
  fputc((Value >> 24) & 0xFF, m_File);
}

//=============================================================================================================================================================================

void xWaveWriter::xWriteHeader()
{
  const uint16_t AudioFormat = 1; // PCM
  const uint16_t BlockAlign = uint16_t((m_NumChannels * m_BitsPerSample) / 8);
  const uint32_t ByteRate = m_SampleRate * BlockAlign;

  // RIFF chunk
  fwrite("RIFF", 1, 4, m_File);
  xWriteLE32(0); // placeholder for RIFF chunk size
  fwrite("WAVE", 1, 4, m_File);

  // fmt chunk
  fwrite("fmt ", 1, 4, m_File);
  xWriteLE32(16); // PCM fmt chunk size
  xWriteLE16(AudioFormat);
  xWriteLE16(m_NumChannels);
  xWriteLE32(m_SampleRate);
  xWriteLE32(ByteRate);
  xWriteLE16(BlockAlign);
  xWriteLE16(m_BitsPerSample);

  // data chunk
  fwrite("data", 1, 4, m_File);
  xWriteLE32(0); // placeholder for data size
}

//=============================================================================================================================================================================

void xWaveWriter::xUpdateHeader()
{
  if(m_File == nullptr)
  {
    return;
  }

  const uint32_t RIFFChunkSize = 36 + m_DataBytesWritten;

  // RIFF size at byte offset 4
  fseek(m_File, 4, SEEK_SET);
  xWriteLE32(RIFFChunkSize);

  // data size at byte offset 40
  fseek(m_File, 40, SEEK_SET);
  xWriteLE32(m_DataBytesWritten);

  // return to end
  fseek(m_File, 0, SEEK_END);
}

//=============================================================================================================================================================================