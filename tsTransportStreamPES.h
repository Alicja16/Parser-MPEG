#pragma once
#include "tsCommon.h"
#include "tsTS.h"
#include "tsTransportStreamHeader.h"
#include "tsTransportStreamAF.h"
#include <string>

//=============================================================================================================================================================================
// MPEG-TS Payload -> PES packet:
//
// Packet start code prefix              (PSCP) : 24 bits  // always 0x000001
// Stream id                             (SID ) : 8 bits
// PES packet length                     (PL  ) : 16 bits
//                                              ----------
//                                                6 bytes

class xPES_PacketHeader
// PES packet
// ├── 6 bajtów podstawowego nagłówka
// │   ├── packet_start_code_prefix   3 B
// │   ├── stream_id                  1 B
// │   └── PES_packet_length          2 B
// │
// ├── 3 bajty początku opcjonalnego nagłówka
// │   ├── flags byte 1               1 B
// │   ├── flags byte 2               1 B
// │   └── PES_header_data_length     1 B
// │
// ├── PES_header_data_length bajtów dodatkowych danych nagłówka
// │   ├── PTS, jeśli jest            5 B
// │   ├── DTS, jeśli jest            5 B
// │   ├── ESCR, jeśli jest
// │   ├── ES_rate, jeśli jest
// │   └── stuffing nagłówka
// │
// └── PES_packet_data
//     └── dane audio/video
{
public:
enum eStreamId : uint8_t
    {
        eStreamId_program_stream_map = 0xBC,
        eStreamId_padding_stream = 0xBE,
        eStreamId_private_stream_2 = 0xBF,
        eStreamId_ECM = 0xF0,
        eStreamId_EMM = 0xF1,
        eStreamId_program_stream_directory = 0xFF,
        eStreamId_DSMCC_stream = 0xF2,
        eStreamId_ITUT_H222_1_type_E = 0xF8,
    };

protected:
    //=================================================================================================================
    // PES packet header - basic mandatory part, 6 bytes
    //=================================================================================================================
    uint32_t m_PacketStartCodePrefix;    // 24 always 0x000001
    uint8_t  m_StreamId;                 //  8 
    uint16_t m_PacketLength;             // 16 


    //=================================================================================================================
    // Optional PES header - first flags byte, present for normal audio/video PES packets
    //=================================================================================================================
    // '10'                              //  2 fixed value
    uint8_t m_PESScramblingControl;      //  2 PES_scrambling_control
    uint8_t m_PESPriority;               //  1 PES_priority
    uint8_t m_DataAlignmentIndicator;    //  1 data_alignment_indicator
    uint8_t m_Copyright;                 //  1 copyright
    uint8_t m_OriginalOrCopy;            //  1 original_or_copy


    //=================================================================================================================
    // Optional PES header - second flags byte
    //=================================================================================================================
    uint8_t m_PTSDTSFlags;               //  2 
    uint8_t m_ESCRFlag;                  //  1 
    uint8_t m_ESRateFlag;                //  1 
    uint8_t m_DSMTrickModeFlag;          //  1 
    uint8_t m_AdditionalCopyInfoFlag;    //  1 
    uint8_t m_PESCRCFlag;                //  1 
    uint8_t m_PESExtensionFlag;          //  1 


    //=================================================================================================================
    // Optional PES header - header data length
    //=================================================================================================================
    uint8_t m_PESHeaderDataLength;       //  8 

    //=================================================================================================================
    // Optional timing fields
    // [PTS and DTS are 33-bit timestamps stored in 5 bytes each [90 kHz clock]]
    //
    // PTS_DTS_flags:
    // 00 - no PTS/DTS
    // 01 - forbidden
    // 10 - PTS only
    // 11 - PTS and DTS
    //=================================================================================================================
    uint64_t m_PTS;                      // 33 kiedy prezentować
    uint64_t m_DTS;                      // 33 kiedy dekodować
    bool     m_HasPTS;                   //    derived flag, true if PTS is present
    bool     m_HasDTS;                   //    derived flag, true if DTS is present

    //=================================================================================================================
    // Derived values 
    // 6 = packet_start_code_prefix + stream_id + PES_packet_length
    // 3 = flags byte + flags byte + PES_header_data_length byte
    //=================================================================================================================
    uint32_t m_HeaderLength;             //  8

public:
    void    Reset();
    int32_t Parse(const uint8_t* Input);
    void    Print() const;

protected:
    bool     hasOptionalPESHeader() const;
    uint64_t xParseTimestamp(const uint8_t* Data) const;

public:
    //PES packet header
    uint32_t getPacketStartCodePrefix() const { return m_PacketStartCodePrefix; }
    uint8_t getStreamId () const { return m_StreamId; }
    uint16_t getPacketLength () const { return m_PacketLength; }

    // Optional PES header
    uint8_t  getPESHeaderDataLength() const { return m_PESHeaderDataLength; }
    uint8_t  getPTSDTSFlags        () const { return m_PTSDTSFlags; }

    // Timing
    bool     hasPTS                () const { return m_HasPTS; }
    bool     hasDTS                () const { return m_HasDTS; }
    uint64_t getPTS                () const { return m_PTS; }
    uint64_t getDTS                () const { return m_DTS; }

    // Derived values
    uint32_t getHeaderLength       () const { return m_HeaderLength; }

    // Whole PES length counted from packet_start_code_prefix.
    // If PES_packet_length == 0, length is unspecified.
    uint32_t getFullPacketLength() const
    {
        return m_PacketLength == 0 ? 0 : uint32_t(xTS::PES_HeaderLength + m_PacketLength);
    }

    // PES packet data length.
    // For PES_packet_length == 0 this returns 0, because size is unspecified.
    uint32_t getDataLength() const
    {
        uint32_t FullPacketLength = getFullPacketLength();

        if(FullPacketLength == 0 || FullPacketLength < m_HeaderLength)
        {
            return 0;
        }

        return FullPacketLength - m_HeaderLength;
    }
};



//=============================================================================================================================================================================

class xPES_Assembler
{
public:
    enum class eResult : int32_t
    {
        UnexpectedPID = 1,
        StreamPacketLost ,
        AssemblingStarted ,
        AssemblingContinue,
        AssemblingFinished,
    };

protected:
    //setup
    int32_t m_PID;

    //buffer
    uint8_t* m_Buffer;
    uint32_t m_BufferSize;
    uint32_t m_DataOffset; // ile bajtów aktualnego PES-a już wpisaliśmy do m_Buffer

    //operation
    int8_t m_LastContinuityCounter;

    // false -> jeszcze nie zaczęliśmy albo właśnie skończyliśmy
    // true  -> mamy rozpoczęty PES i czekamy na kolejne fragmenty
    bool m_Started;

    xPES_PacketHeader m_PESH; // nagłówek aktualnego PES


public:
    xPES_Assembler (); // konstruktor - ustawić obiekt w bezpieczny stan początkowy
    ~xPES_Assembler(); // destruktor - zwolnić pamięc bufora
    void Init (int32_t PID); // skłądasz PESy o danym PID

    eResult AbsorbPacket( //weź jeden pakiet TS i spróbuj wchłonąć jego payload do aktualnego PES
        const uint8_t* TransportStreamPacket,
        const xTS_PacketHeader* PacketHeader,
        const xTS_AdaptationField* AdaptationField
    );

    void PrintPESH () const { m_PESH.Print(); } // pokaż nagłówek PES

    uint8_t* getPacket () { return m_Buffer; } // daj złożony PES
    int32_t getNumPacketBytes() const { return m_DataOffset; } // daj rozmiar złożonego PES

    const xPES_PacketHeader& getPESH() const { return m_PESH; } // daj informacje z nagłówka PES

    void PrintResult(eResult Result) const;

protected:
    void xBufferReset();
    void xBufferAppend(const uint8_t* Data, int32_t Size);
};