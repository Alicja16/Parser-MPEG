//=============================================================================================================================================================================
#include "tsTransportStreamPES.h"
#include "tsTS.h"
#include <iostream>
#include <iomanip>
#include <cstring>

//=============================================================================================================================================================================
// xPES_PacketHeader
//=============================================================================================================================================================================
void xPES_PacketHeader::Reset()
{
    //=================================================================================================================
    // PES packet header - mandatory     6 bytes
    //=================================================================================================================
    m_PacketStartCodePrefix = 0;
    m_StreamId              = 0;
    m_PacketLength          = 0;

    //=================================================================================================================
    // Optional PES header - first flags 1 byte
    //=================================================================================================================
    m_PESScramblingControl   = 0;
    m_PESPriority            = 0;
    m_DataAlignmentIndicator = 0;
    m_Copyright              = 0;
    m_OriginalOrCopy         = 0;

    //=================================================================================================================
    // Optional PES header - second flags 1 byte
    //=================================================================================================================
    m_PTSDTSFlags            = 0;
    m_ESCRFlag               = 0;
    m_ESRateFlag             = 0;
    m_DSMTrickModeFlag       = 0;
    m_AdditionalCopyInfoFlag = 0;
    m_PESCRCFlag             = 0;
    m_PESExtensionFlag       = 0;

    //=================================================================================================================
    // Optional PES header - header data 1 byte
    //=================================================================================================================
    m_PESHeaderDataLength    = 0;

    //=================================================================================================================
    // Optional timing fields   10 bytes
    //=================================================================================================================
    m_PTS                    = 0;
    m_DTS                    = 0;
    m_HasPTS                 = false;
    m_HasDTS                 = false;

    //=================================================================================================================
    // Derived values   1 byte
    //=================================================================================================================
    m_HeaderLength           = xTS::PES_HeaderLength;
}

bool xPES_PacketHeader::hasOptionalPESHeader() const
{
    if(m_StreamId == eStreamId_program_stream_map)       return false;
    if(m_StreamId == eStreamId_padding_stream)           return false;
    if(m_StreamId == eStreamId_private_stream_2)         return false;
    if(m_StreamId == eStreamId_ECM)                      return false;
    if(m_StreamId == eStreamId_EMM)                      return false;
    if(m_StreamId == eStreamId_program_stream_directory) return false;
    if(m_StreamId == eStreamId_DSMCC_stream)             return false;
    if(m_StreamId == eStreamId_ITUT_H222_1_type_E)       return false;

    return true;
}


uint64_t xPES_PacketHeader::xParseTimestamp(const uint8_t* Data) const
{
    uint64_t TimeStamp =
        (uint64_t(Data[0] & 0x0E) << 29) |
        (uint64_t(Data[1])        << 22) |
        (uint64_t(Data[2] & 0xFE) << 14) |
        (uint64_t(Data[3])        <<  7) |
        (uint64_t(Data[4] & 0xFE) >>  1);

    return TimeStamp;
}



int32_t xPES_PacketHeader::Parse(const uint8_t* Input)
{
    if(Input == nullptr){ return NOT_VALID;}
    Reset();

    //=================================================================================================================
    // PES packet header - mandatory 6 bytes
    //
    // byte 0..2 : packet_start_code_prefix   24 bits
    // byte 3    : stream_id                   8 bits
    // byte 4..5 : PES_packet_length          16 bits
    //=================================================================================================================

    // [23][22][21][20][19][18][17][16] [15][14][13][12][11][10][ 9][ 8] [ 7][ 6][ 5][ 4][ 3][ 2][ 1][ 0]
    //     Input[0]                         Input[1]                         Input[2]
    m_PacketStartCodePrefix =
        (uint32_t(Input[0]) << 16) |
        (uint32_t(Input[1]) <<  8) |
        (uint32_t(Input[2])      );


    m_StreamId = Input[3];


    // [15][14][13][12][11][10][ 9][ 8] [ 7][ 6][ 5][ 4][ 3][ 2][ 1][ 0]
    //     Input[4]                         Input[5]
    m_PacketLength =
        (uint16_t(Input[4]) << 8) |
        (uint16_t(Input[5])     );

    // packet_start_code_prefix must be 0x000001
    if(m_PacketStartCodePrefix != 0x000001) {return NOT_VALID;}


    if(!hasOptionalPESHeader())
    {
        m_HeaderLength = xTS::PES_HeaderLength;
        return int32_t(m_HeaderLength);
    }

    //=================================================================================================================
    // Optional PES header - first 3 bytes after mandatory PES header
    //
    // byte 6:
    // '10'                         2 bits
    // PES_scrambling_control       2 bits
    // PES_priority                 1 bit
    // data_alignment_indicator     1 bit
    // copyright                    1 bit
    // original_or_copy             1 bit
    //
    // byte 7:
    // PTS_DTS_flags                2 bits
    // ESCR_flag                    1 bit
    // ES_rate_flag                 1 bit
    // DSM_trick_mode_flag          1 bit
    // additional_copy_info_flag    1 bit
    // PES_CRC_flag                 1 bit
    // PES_extension_flag           1 bit
    //
    // byte 8:
    // PES_header_data_length       8 bits
    //=================================================================================================================

    uint8_t Byte6 = Input[6];
    // Byte6 = [7][6][5][4][3][2][1][0]
    //      |  |  |  |  |  |  |  |
    //      |  |  |  |  |  |  |  +-- original_or_copy
    //      |  |  |  |  |  |  +----- copyright
    //      |  |  |  |  |  +-------- data_alignment_indicator
    //      |  |  |  |  +----------- PES_priority
    //      |  |  +--+-------------- PES_scrambling_control
    //      +--+-------------------- fixed bits = '10'

    uint8_t FixedBits = uint8_t(
        (Byte6 & 0xC0) >> 6
    );

    if(FixedBits != 0b10){ return NOT_VALID; }

    m_PESScramblingControl   = uint8_t((Byte6 & 0x30) >> 4);
    m_PESPriority            = uint8_t((Byte6 & 0x08) >> 3);
    m_DataAlignmentIndicator = uint8_t((Byte6 & 0x04) >> 2);
    m_Copyright              = uint8_t((Byte6 & 0x02) >> 1);
    m_OriginalOrCopy         = uint8_t((Byte6 & 0x01)     );



    uint8_t Byte7 = Input[7];
    // Byte7 = [7][6][5][4][3][2][1][0]
    //          |  |  |  |  |  |  |  |
    //          |  |  |  |  |  |  |  +-- PES_extension_flag
    //          |  |  |  |  |  |  +----- PES_CRC_flag
    //          |  |  |  |  |  +-------- additional_copy_info_flag
    //          |  |  |  |  +----------- DSM_trick_mode_flag
    //          |  |  |  +-------------- ES_rate_flag
    //          |  |  +----------------- ESCR_flag
    //          +--+-------------------- PTS_DTS_flags

    m_PTSDTSFlags            = uint8_t((Byte7 & 0xC0) >> 6);
    m_ESCRFlag               = uint8_t((Byte7 & 0x20) >> 5);
    m_ESRateFlag             = uint8_t((Byte7 & 0x10) >> 4);
    m_DSMTrickModeFlag       = uint8_t((Byte7 & 0x08) >> 3);
    m_AdditionalCopyInfoFlag = uint8_t((Byte7 & 0x04) >> 2);
    m_PESCRCFlag             = uint8_t((Byte7 & 0x02) >> 1);
    m_PESExtensionFlag       = uint8_t((Byte7 & 0x01)     );

   

    //=================================================================================================================
    // Full PES header length for normal audio/video PES:
    //
    // 6 = mandatory PES header
    // 3 = {
    //      byte 6      pierwszy bajt flag
    //      byte 7      drugi bajt flag
    //      byte 8      PES_header_data_length
    //  }
    // PES_header_data_length = optional fields after byte 8
    //=================================================================================================================
    m_PESHeaderDataLength = Input[8];
    m_HeaderLength = xTS::PES_HeaderLength + 3 + m_PESHeaderDataLength;


    //=================================================================================================================
    // Optional timing fields: PTS / DTS
    //
    // Input[0]   packet_start_code_prefix, bajt 1
    // Input[1]   packet_start_code_prefix, bajt 2
    // Input[2]   packet_start_code_prefix, bajt 3
    // Input[3]   stream_id
    // Input[4]   PES_packet_length, bajt starszy
    // Input[5]   PES_packet_length, bajt młodszy

    // Input[6]   pierwszy bajt flag optional PES header
    // Input[7]   drugi bajt flag optional PES header
    // Input[8]   PES_header_data_length

    // Input[9]   tu zaczynają się dodatkowe pola nagłówka, np. PTS/DTS
    //=================================================================================================================

    uint32_t Offset = xTS::PES_HeaderLength + 3;
    uint32_t HeaderBytesLeft = m_PESHeaderDataLength;

    // PTS_DTS_flags:
    // 00 - no PTS/DTS
    // 01 - forbidden
    // 10 - PTS only
    // 11 - PTS and DTS

    if(m_PTSDTSFlags == 0b01){ return NOT_VALID;}

    if(m_PTSDTSFlags == 0b10)
    {
        
        if(HeaderBytesLeft < 5){ return NOT_VALID; }// PTS: 5 bytes

        m_PTS = xParseTimestamp(Input + Offset);
        m_HasPTS = true;

        Offset += 5;
        HeaderBytesLeft -= 5;
    }

    else if(m_PTSDTSFlags == 0b11)
    {
        
        if(HeaderBytesLeft < 10){ return NOT_VALID; }// PTS + DTS: 5 bytes + 5 bytes

        m_PTS = xParseTimestamp(Input + Offset);
        m_HasPTS = true;

        Offset += 5;
        HeaderBytesLeft -= 5;

        m_DTS = xParseTimestamp(Input + Offset);
        m_HasDTS = true;

        Offset += 5;
        HeaderBytesLeft -= 5;
    }

    return int32_t(m_HeaderLength);
}


void xPES_PacketHeader::Print() const
{
    std::cout << "PES: "
              << "PSCP=" << m_PacketStartCodePrefix
              << " SID=" << unsigned(m_StreamId)
              << " L=" << m_PacketLength
              << " HeadLen=" << m_HeaderLength;

    if(m_HasPTS)
    {
        double PTSTime = double(m_PTS) / double(xTS::BaseClockFrequency_Hz);

        std::cout << " PTS=" << m_PTS
                  << " (Time=" << std::fixed << std::setprecision(6)
                  << PTSTime << "s)";
    }

    if(m_HasDTS)
    {
        double DTSTime = double(m_DTS) / double(xTS::BaseClockFrequency_Hz);

        std::cout << " DTS=" << m_DTS
                  << " (Time=" << std::fixed << std::setprecision(6)
                  << DTSTime << "s)";
    }
}


//=============================================================================================================================================================================
// xPES_Assembler
//=============================================================================================================================================================================

xPES_Assembler::xPES_Assembler()
{
    m_PID = -1;

    m_Buffer = nullptr;
    m_BufferSize = 0;
    m_DataOffset = 0;

    m_LastContinuityCounter = -1;
    m_Started = false;

    m_PESH.Reset();
}


xPES_Assembler::~xPES_Assembler()
{
    delete[] m_Buffer;
    m_Buffer = nullptr;
}

void xPES_Assembler::Init(int32_t PID)
{
    m_PID = PID;

    m_BufferSize = 1024 * 1024; // 1 MB startowego bufora

    delete[] m_Buffer;
    m_Buffer = new uint8_t[m_BufferSize];

    m_DataOffset = 0;
    m_LastContinuityCounter = -1; // CC
    m_Started = false;

    m_PESH.Reset();
}


void xPES_Assembler::xBufferReset()
{
    m_DataOffset = 0;
    m_PESH.Reset();
}


void xPES_Assembler::xBufferAppend(const uint8_t* Data, int32_t Size)
{
    if(Data == nullptr || Size <= 0){ return; }

    // czy zmiecimy sie w buforze, jeli nie -> podwój bufor
    if(m_DataOffset + uint32_t(Size) > m_BufferSize)
    {
        uint32_t NewBufferSize = m_BufferSize;

        while(m_DataOffset + uint32_t(Size) > NewBufferSize)
        {
            NewBufferSize *= 2;
        }


        uint8_t* NewBuffer = new uint8_t[NewBufferSize];
        if(m_Buffer != nullptr && m_DataOffset > 0)
        {
            // kopia starych danych do nowego bufora
            memcpy(NewBuffer, m_Buffer, m_DataOffset);
        }

        // usuniecie starego
        delete[] m_Buffer;

        // podpiecie nowego
        m_Buffer = NewBuffer;
        m_BufferSize = NewBufferSize;
    }

    memcpy(m_Buffer + m_DataOffset, Data, Size);
    m_DataOffset += Size;
}


xPES_Assembler::eResult xPES_Assembler::AbsorbPacket(
    const uint8_t* TransportStreamPacket,
    const xTS_PacketHeader* PacketHeader,
    const xTS_AdaptationField* AdaptationField
)
{   
    // TransportStreamPacket == nullptr -> nie mamy 188 bajtów pakietu TS
    // PacketHeader == nullptr -> nie mamy informacji o PID, S, AFC, CC itd
    if(TransportStreamPacket == nullptr || PacketHeader == nullptr)
    {
        return eResult::StreamPacketLost;
    }

    //=================================================================================================================
    // 1. PID filtering
    //
    // This assembler works only for one selected PID, e.g. PID=136 for audio.
    // If the current TS packet has a different PID, it is ignored.
    //=================================================================================================================
    if(PacketHeader->getPacketIdentifier() != m_PID)
    {
        return eResult::UnexpectedPID;
    }

    //=================================================================================================================
    // 2. Payload check
    //
    // PES data is carried in TS payload.
    // If this TS packet has no payload, there is nothing to append to PES.
    //=================================================================================================================
    if(!PacketHeader->hasPayload())
    {
        return eResult::AssemblingContinue;
    }

    //=================================================================================================================
    // 3. Continuity counter verification
    // For packets with the same PID and with payload, continuity_counter should increment modulo 16:
    //
    // 0, 1, 2, 3, ..., 15, 0, 1, 2, ...
    // If the value is not as expected, it means that one or more TS packets may be missing.
    //=================================================================================================================
    int8_t CurrentCC = int8_t(PacketHeader->getContinuityCounter());

    if(m_LastContinuityCounter >= 0)
    {
        int8_t ExpectedCC = int8_t((m_LastContinuityCounter + 1) & 0x0F); // 15 -> 0

        if(CurrentCC == m_LastContinuityCounter)
        {
            // Duplicate packet - do not append the same payload again.
            return eResult::AssemblingContinue;
        }

        if(CurrentCC != ExpectedCC)
        {
            m_Started = false;
            xBufferReset();
            m_LastContinuityCounter = CurrentCC;

            return eResult::StreamPacketLost;
        }
    }
    m_LastContinuityCounter = CurrentCC;


    //=================================================================================================================
    // 4. Payload offset inside TS packet (188 bytes)
    // [ TS header ][ optional adaptation field ][ payload ]
    //
    // TS header (4 bytes)
    // If there is no adaptation field: payload starts at byte 4.
    // If adaptation field exists: payload starts at byte 4 + adaptation_field_size.
    //=================================================================================================================
    uint32_t PayloadOffset = xTS::TS_HeaderLength;

    if(PacketHeader->hasAdaptationField())
    {
        if(AdaptationField == nullptr)
        {
            return eResult::StreamPacketLost;
        }

        PayloadOffset += AdaptationField->getNumBytes();
    }

    if(PayloadOffset >= xTS::TS_PacketLength)
    {
        return eResult::StreamPacketLost;
    }

    uint32_t PayloadSize = xTS::TS_PacketLength - PayloadOffset;


    //=================================================================================================================
    // 5. Start of a new PES packet
    //
    // payload_unit_start_indicator == 1 means that payload of this TS packet starts with a new PES packet.
    //
    // Therefore:
    // - reset PES buffer,
    // - parse PES header from the beginning of payload,
    // - append payload bytes to PES buffer,
    // - check whether the whole PES is already complete.
    //=================================================================================================================
    if(PacketHeader->getPayloadUnitStartIndicator() == 1)
    {
        xBufferReset();
        m_Started = true;

        if(PayloadSize < xTS::PES_HeaderLength)
        {
            m_Started = false;
            return eResult::StreamPacketLost;
        }

        int32_t HeaderLength = m_PESH.Parse(TransportStreamPacket + PayloadOffset);

        if(HeaderLength == NOT_VALID)
        {
            m_Started = false;
            return eResult::StreamPacketLost;
        }

        uint32_t ExpectedPESSize = m_PESH.getFullPacketLength();

        // If PES_packet_length != 0, we know exact PES size.
        // Do not append bytes beyond this PES.
        if(ExpectedPESSize != 0 && PayloadSize > ExpectedPESSize)
        {
            PayloadSize = ExpectedPESSize;
        }

        xBufferAppend(TransportStreamPacket + PayloadOffset, int32_t(PayloadSize));

        if(ExpectedPESSize != 0 && m_DataOffset >= ExpectedPESSize)
        {
            m_Started = false;
            return eResult::AssemblingFinished;
        }

        return eResult::AssemblingStarted;
    }
    

    //=================================================================================================================
    // 6. Continuation of current PES packet
    //
    // If payload_unit_start_indicator == 0, this TS payload is a continuation
    // of the PES packet that has already been started earlier.
    //=================================================================================================================
    if(!m_Started)
    {
        return eResult::StreamPacketLost;
    }

    uint32_t ExpectedPESSize = m_PESH.getFullPacketLength();

    if(ExpectedPESSize != 0)
    {
        if(m_DataOffset >= ExpectedPESSize)
        {
            m_Started = false;
            return eResult::AssemblingFinished;
        }

        uint32_t BytesLeft = ExpectedPESSize - m_DataOffset;

        if(PayloadSize > BytesLeft)
        {
            PayloadSize = BytesLeft;
        }
    }

    xBufferAppend(TransportStreamPacket + PayloadOffset, int32_t(PayloadSize));

    if(ExpectedPESSize != 0 && m_DataOffset >= ExpectedPESSize)
    {
        m_Started = false;
        return eResult::AssemblingFinished;
    }

    return eResult::AssemblingContinue;
}


void xPES_Assembler::PrintResult(eResult Result) const
{
    switch(Result)
    {
        case eResult::UnexpectedPID:
            break;

        case eResult::StreamPacketLost:
            std::cout << "  PcktLost ";
            break;

        case eResult::AssemblingStarted:
            std::cout << "  Started ";
            PrintPESH();
            break;

        case eResult::AssemblingContinue:
            std::cout << "  Continue ";
            break;

        case eResult::AssemblingFinished:
            std::cout << "  Finished PES: Len=" << getNumPacketBytes();
            break;

        default:
            break;
    }
}