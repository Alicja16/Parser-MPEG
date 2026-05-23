#include "tsCommon.h"
#include "tsTS.h"
#include "tsTransportStreamAF.h"
#include "tsTransportStreamHeader.h"
#include "tsPESPacketHeader.h"
#include "tsPESAssembler.h"
#include <cstdio>
#include <cstdlib>

//=============================================================================================================================================================================

int main()
{
  // open input file
  FILE* TS_File = fopen("example_new.ts", "rb");

  // check if input file is opened
  if(TS_File == nullptr)
  {
    printf("ERROR: cannot open input file\n");
    return EXIT_FAILURE;
  }

  // open result file
  FILE* RESULT_File = fopen("PID136.mp2", "wb");

  // check if result file is opened
  if(RESULT_File == nullptr)
  {
    printf("ERROR: cannot open result file\n");
    fclose(TS_File);
    return EXIT_FAILURE;
  }

  // buffer for one TS packet
  uint8_t TS_PacketBuffer[xTS::TS_PacketLength]; // 188 bytes

  // packet parsing class objects
  xTS_PacketHeader    TS_PacketHeader;
  xTS_AdaptationField TS_PacketAdaptationField;

  // PES assembler for audio PID = 136
  xPES_Assembler PES_Assembler;
  PES_Assembler.Init(136);

  // number of packet
  int32_t TS_PacketId = 0;

  while(true)
  {
    // read one TS packet from file
    size_t NumRead = fread(TS_PacketBuffer, 1, xTS::TS_PacketLength, TS_File);

    if(NumRead != xTS::TS_PacketLength)
    {
      break;
    }

    // parse TS header
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

    // we are interested only in audio PID = 136
    if(TS_PacketHeader.getPacketIdentifier() == 136)
    {
      // parse Adaptation Field only if it exists
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
        }
      }
    }

    TS_PacketId++;
  }

  // close files
  fclose(RESULT_File);
  fclose(TS_File);

  return EXIT_SUCCESS;
}

//=============================================================================================================================================================================