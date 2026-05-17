#pragma once
#include "tsCommon.h"
#include <string>

/*
MPEG-TS packet:
`        3                   2                   1                   0  `
`      1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0  `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `
`   0 |                             Header                            | `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `
`   4 |                  Adaptation field + Payload                   | `
`     |                                                               | `
` 184 |                                                               | `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `


MPEG-TS packet header: (4 bytes = 32 bits)
`        3                   2                   1                   0  `
`      1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0  `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `
`   0 |       SB      |E|S|T|           PID           |TSC|AFC|   CC  | `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `

Sync byte                    (SB ) :  8 bits [0x47 - start of the package]
Transport error indicator    (E  ) :  1 bit  [1 - uncorrectable bit error was detected - suspicious/damaged]
Payload unit start indicator (S  ) :  1 bit  [0 - whether a new logical data unit begins in the payload]
Transport priority           (T  ) :  1 bit  [1 - higher priority than other packets with the same PID]
Packet Identifier            (PID) : 13 bits [the type of data stored in the package payload]
Transport scrambling control (TSC) :  2 bits [whether the *!payload!* of the package is encrypted/scrambled - (01, 10, 11 - user-defined), (00 - not scrambled)]
Adaptation field control     (AFC) :  2 bits [what comes after the headline]
Continuity counter           (CC ) :  4 bits [packets with the same PID (0-15)]



MPEG-TS adaptation field:`
Adaptation field length               (AFL) : 8 bits [how many bytes are there right after it inside the adaptation field]
`                               
`                    AFL > 0
`   ┌─────────────────────────────────────┐
`   |-------DC-------|RA|SP|PR|OR|SF|TP|EX|  ← 1 byte of flags
`   └─────────────────────────────────────┘
`
Discontinuity indicator               (DC ) : 1 bit  [continuity counter or time base discontinuities]
Random access indicator               (RA ) : 1 bit  [information useful for random access (I-frame area/access point)]
Elementary stream priority indicator  (SP ) : 1 bit  [payload has higher priority than other data]
Program Clock Reference flag          (PR ) : 1 bit  [1 - after the flag byte there is a PCR field]                               
`   ┌──────────────────────────────────────────────────────────────┐
`   |----------- PCR_base ----------|reserved|--- PCR_extension ---|  ← 6 bytes of PCR
`   |           33 bity             | 6 bitów|       9 bitów       |
`   └──────────────────────────────────────────────────────────────┘
`
Original Program Clock Reference flag (OR ) : 1 bit  [1 - after the flag byte there is a OPCR field (can only occur with PCR)]                        
`   ┌──────────────────────────────────────────────────────────────┐
`   |----------- OPCR_base ---------|reserved|--- OPCR_extension --|  ← 6 bytes of OPCR
`   |           33 bity             | 6 bitów|       9 bitów       |
`   └──────────────────────────────────────────────────────────────┘
`
Splicing point flag                   (SF ) : 1 bit  [1 - splice_countdown (1 byte) - stream switching/advertising]             
`   ┌────────────────────────────┐
`   |----- splice countdown -----|  ← 1 byte of splice countdown
`   └────────────────────────────┘
`
Transport private data flag           (TP ) : 1 bit  [any manufacturer's data]
`   ┌──────────────────────────────────────┐
`   │------------TP_data_length------------│ 8 bitów 
`   ├──────────────────────────────────────┤
`   │ private_data_byte[0]                 │ 8 bitów
`   │ private_data_byte[1]                 │ 8 bitów
`   │ ...                                  │
`   │ private_data_byte[N-1]               │ 8 bitów
`   └──────────────────────────────────────┘
`
Adaptation field extension flag       (EX ) : 1 bit
`                     EX = 1:
`
`    adaptation_field_extension_length → 8 bitów
`
`    ltw_flag                          → 1 bit
`    piecewise_rate_flag               → 1 bit
`    seamless_splice_flag              → 1 bit
`    reserved                          → 5 bitów
`
`    if (ltw_flag):
`        ltw_valid_flag                → 1 bit
`        ltw_offset                    → 15 bitów
`
`    if (piecewise_rate_flag):
`        reserved                      → 2 bity
`        piecewise_rate                → 22 bity
`
`    if (seamless_splice_flag):
`        splice_type                   → 4 bity
`        DTS_next_AU                   → 33 bity (rozbite)
`        marker_bits                   → 3 bity
`
*/


//=============================================================================================================================================================================

class xTS
{
public:
  static constexpr uint32_t TS_PacketLength  = 188;
  static constexpr uint32_t TS_HeaderLength  = 4;

  static constexpr uint32_t PES_HeaderLength = 6;

  static constexpr uint32_t BaseClockFrequency_Hz         =    90000; //Hz
  static constexpr uint32_t ExtendedClockFrequency_Hz     = 27000000; //Hz
  static constexpr uint32_t BaseClockFrequency_kHz        =       90; //kHz
  static constexpr uint32_t ExtendedClockFrequency_kHz    =    27000; //kHz
  static constexpr uint32_t BaseToExtendedClockMultiplier =      300;
};

//=============================================================================================================================================================================
