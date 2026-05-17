#include "tsCommon.h"
#include "tsTS.h"
#include "tsTransportStreamAF.h"
#include "tsTransportStreamHeader.h"
#include "tsTransportStreamPES.h"
#include <cstdio>
#include <cstdlib>

//=============================================================================================================================================================================

int main()
{
  // open file
  FILE* TS_File = fopen("example_new.ts", "rb");

  // check if file if opened
  if (TS_File == nullptr)
  {
    printf("ERROR: cannot open input file\n");
    return EXIT_FAILURE;
  }
  // result file
  FILE* RESULT_File = fopen("PID136.mp2", "wb");


  // buffer
  uint8_t TS_PacketBuffer[xTS::TS_PacketLength]; // ← 188 bytes

  // package parsing class object
  xTS_PacketHeader    TS_PacketHeader;
  xTS_AdaptationField TS_PacketAdaptationField;

  // PES assembler for audio PID = 136
  xPES_Assembler PES_Assembler;
  PES_Assembler.Init(136);

  // Number of packet
  int32_t TS_PacketId = 0;
  while(!feof(TS_File))
  {
    // read from file
    size_t NumRead = fread(TS_PacketBuffer, 1, xTS::TS_PacketLength, TS_File);

    if (NumRead != xTS::TS_PacketLength) break;

    TS_PacketHeader.Reset();
    TS_PacketHeader.Parse(TS_PacketBuffer);
    

    TS_PacketAdaptationField.Reset();
    int32_t AF_NumBytes = TS_PacketAdaptationField.Parse(
      TS_PacketBuffer,
      TS_PacketHeader.getAdaptationFieldControl()
    );
    


    if(TS_PacketHeader.getPacketIdentifier() == 136)
    {
      xPES_Assembler::eResult PES_Result = PES_Assembler.AbsorbPacket(
        TS_PacketBuffer,
        &TS_PacketHeader,
        &TS_PacketAdaptationField
      );

      printf("%010d ", TS_PacketId);
      TS_PacketHeader.Print();
      if(AF_NumBytes > 0) TS_PacketAdaptationField.Print();
      PES_Assembler.PrintResult(PES_Result);
      printf("\n");

      if(PES_Result == xPES_Assembler::eResult::AssemblingFinished)
      {
        const xPES_PacketHeader& PESH = PES_Assembler.getPESH();

        uint8_t* PES_Data = PES_Assembler.getPacket();

        uint32_t HeaderLen = PESH.getHeaderLength();
        uint32_t DataLen   = PESH.getDataLength();

        fwrite(PES_Data + HeaderLen, 1, DataLen, RESULT_File);
      }

    }
    TS_PacketId++;
  }
  
  // close file
  fclose(TS_File);

  return EXIT_SUCCESS;
}

//=============================================================================================================================================================================
