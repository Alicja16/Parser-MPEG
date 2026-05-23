#include "tsCommon.h"
#include "tsTS.h"
#include "tsTransportStreamAF.h"
#include "tsTransportStreamHeader.h"
#include "tsPESPacketHeader.h"
#include "tsPESAssembler.h"
#include "tsMPG123Decoder.h"

#include <cstdio>
#include <cstdlib>
#include <cstdint>

//=============================================================================================================================================================================

int main()
{
  //===========================================================================================================================================================================
  // Files
  //===========================================================================================================================================================================

  // open input file
  FILE* TS_File = fopen("example_new.ts", "rb");

  // check if input file is opened
  if(TS_File == nullptr)
  {
    printf("ERROR: cannot open input file\n");
    return EXIT_FAILURE;
  }

  // open audio result file
  FILE* RESULT_File = fopen("PID136.mp2", "wb");

  // check if audio result file is opened
  if(RESULT_File == nullptr)
  {
    printf("ERROR: cannot open audio result file\n");
    fclose(TS_File);
    return EXIT_FAILURE;
  }

  // open video result file
  FILE* VIDEO_File = fopen("PID174.264", "wb");

  // check if video result file is opened
  if(VIDEO_File == nullptr)
  {
    printf("ERROR: cannot open video result file\n");
    fclose(RESULT_File);
    fclose(TS_File);
    return EXIT_FAILURE;
  }

  //===========================================================================================================================================================================
  // Setup
  //===========================================================================================================================================================================

  const int32_t AudioPID = 136;
  const int32_t VideoPID = 174;

  // buffer for one TS packet
  uint8_t TS_PacketBuffer[xTS::TS_PacketLength]; // 188 bytes

  // packet parsing class objects
  xTS_PacketHeader    TS_PacketHeader;
  xTS_AdaptationField TS_PacketAdaptationField;

  // PES assembler for audio PID = 136
  xPES_Assembler PES_Assembler;
  PES_Assembler.Init(AudioPID);

  xMPG123Decoder MPG123Decoder;

  if(!MPG123Decoder.Init("PID136.wav"))
  {
    printf("ERROR: cannot initialize mpg123 decoder\n");

    fclose(VIDEO_File);
    fclose(RESULT_File);
    fclose(TS_File);

    return EXIT_FAILURE;
  }

  // number of packet
  int32_t TS_PacketId = 0;

  //===========================================================================================================================================================================
  // Main loop
  //===========================================================================================================================================================================

  while(true)
  {
    // read one TS packet from file
    size_t NumRead = fread(TS_PacketBuffer, 1, xTS::TS_PacketLength, TS_File);

    if(NumRead != xTS::TS_PacketLength)
    {
      break;
    }

    //=========================================================================================================================================================================
    // Parse TS header
    //=========================================================================================================================================================================

    TS_PacketHeader.Reset();

    int32_t HeaderResult = TS_PacketHeader.Parse(TS_PacketBuffer);

    if(HeaderResult == NOT_VALID)
    {
      printf("%010d ERROR: invalid TS header\n", TS_PacketId);
      TS_PacketId++;
      continue;
    }

    // check sync byte
    if(TS_PacketHeader.getSyncByte() != 0x47)
    {
      printf("%010d ERROR: wrong sync byte\n", TS_PacketId);
      TS_PacketId++;
      continue;
    }

    //=========================================================================================================================================================================
    // Parse Adaptation Field
    //
    // Important:
    // AF must be parsed before both audio and video branches,
    // because both need correct payload offset.
    //=========================================================================================================================================================================

    TS_PacketAdaptationField.Reset();

    int32_t AF_NumBytes = 0;

    if(TS_PacketHeader.hasAdaptationField())
    {
      AF_NumBytes = TS_PacketAdaptationField.Parse(
        TS_PacketBuffer,
        TS_PacketHeader.getAdaptationFieldControl()
      );

      if(AF_NumBytes == NOT_VALID)
      {
        printf("%010d ", TS_PacketId);
        TS_PacketHeader.Print();
        printf(" ERROR: invalid adaptation field\n");

        TS_PacketId++;
        continue;
      }
    }

    //=========================================================================================================================================================================
    // Audio extraction: PID 136 -> PID136.mp2
    //=========================================================================================================================================================================

    if(TS_PacketHeader.getPacketIdentifier() == AudioPID)
    {
      // assemble PES packet
      xPES_Assembler::eResult PES_Result = PES_Assembler.AbsorbPacket(
        TS_PacketBuffer,
        &TS_PacketHeader,
        &TS_PacketAdaptationField
      );

      // print information
      printf("%010d ", TS_PacketId);
      TS_PacketHeader.Print();

      if(AF_NumBytes > 0)
      {
        TS_PacketAdaptationField.Print();
      }

      PES_Assembler.PrintResult(PES_Result);
      printf("\n");

      // if PES packet is complete, write only PES_packet_data_bytes to result file
      if(PES_Result == xPES_Assembler::eResult::AssemblingFinished)
      {
        const uint8_t* PES_Data = PES_Assembler.getPacketData();
        uint32_t DataLen = PES_Assembler.getPacketDataLength();

        if(PES_Data != nullptr && DataLen > 0)
        {
          fwrite(PES_Data, 1, DataLen, RESULT_File);

          if(!MPG123Decoder.Decode(PES_Data, DataLen))
          {
            printf("ERROR: mpg123 decoding failed\n");

            MPG123Decoder.Close();

            fclose(VIDEO_File);
            fclose(RESULT_File);
            fclose(TS_File);

            return EXIT_FAILURE;
          }
        }
      }
    }

    //=========================================================================================================================================================================
    // Video extraction: PID 174 -> PID174.264
    //
    // Option A:
    // - do not use PES assembler for video
    // - write TS payload directly
    // - if PUSI == 1, skip PES header first
    //=========================================================================================================================================================================

    else if(TS_PacketHeader.getPacketIdentifier() == VideoPID)
    {
      printf("%010d ", TS_PacketId);
      TS_PacketHeader.Print();

      if(AF_NumBytes > 0)
      {
        TS_PacketAdaptationField.Print();
      }

      // packet without payload cannot carry PES data
      if(!TS_PacketHeader.hasPayload())
      {
        printf(" VIDEO: NoPayload\n");
        TS_PacketId++;
        continue;
      }

      // calculate payload offset:
      // [4-byte TS header][optional adaptation field][payload]
      uint32_t PayloadOffset = xTS::TS_HeaderLength;

      if(TS_PacketHeader.hasAdaptationField())
      {
        PayloadOffset += TS_PacketAdaptationField.getNumBytes();
      }

      if(PayloadOffset >= xTS::TS_PacketLength)
      {
        printf(" VIDEO: InvalidPayloadOffset\n");
        TS_PacketId++;
        continue;
      }

      const uint8_t* PayloadData = TS_PacketBuffer + PayloadOffset;
      uint32_t PayloadSize = xTS::TS_PacketLength - PayloadOffset;

      // If this packet starts a new PES packet,
      // payload begins with PES header, so skip this PES header.
      if(TS_PacketHeader.getPayloadUnitStartIndicator() == 1)
      {
        xPES_PacketHeader VideoPESH;

        const int32_t PESHeaderLength = VideoPESH.Parse(PayloadData);

        if(PESHeaderLength == NOT_VALID)
        {
          printf(" VIDEO: invalid PES header\n");
          TS_PacketId++;
          continue;
        }

        printf(" VIDEO: Started ");
        VideoPESH.Print();

        if(uint32_t(PESHeaderLength) >= PayloadSize)
        {
          printf(" EmptyPayloadAfterPESHeader\n");
          TS_PacketId++;
          continue;
        }

        PayloadData += PESHeaderLength;
        PayloadSize -= uint32_t(PESHeaderLength);
      }
      else
      {
        printf(" VIDEO: Continue");
      }

      const size_t NumWritten = fwrite(PayloadData, 1, PayloadSize, VIDEO_File);

      if(NumWritten != PayloadSize)
      {
        printf(" ERROR: cannot write video data\n");

        fclose(VIDEO_File);
        fclose(RESULT_File);
        fclose(TS_File);

        return EXIT_FAILURE;
      }

      printf(" WriteVideo=%u\n", PayloadSize);
    }

    TS_PacketId++;
  }

  //===========================================================================================================================================================================
  // Close files
  //===========================================================================================================================================================================
  MPG123Decoder.Close();
  fclose(VIDEO_File);
  fclose(RESULT_File);
  fclose(TS_File);

  return EXIT_SUCCESS;
}

//=============================================================================================================================================================================