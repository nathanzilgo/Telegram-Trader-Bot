//+------------------------------------------------------------------+
//|                                                     MetaTrader 5 |
//|                   Copyright 2000-2019, MetaQuotes Software Corp. |
//|                                        http://www.metaquotes.net |
//+------------------------------------------------------------------+
#pragma once
//--- version info
#define MT5_IPC_CURRENT_VERSION 1
//+------------------------------------------------------------------+
//| IPC call results                                                 |
//+------------------------------------------------------------------+
enum EnMT5IPCResult : INT32
  {
   MT5_IPC_RESULT_OK  =1,
   MT5_IPC_RESULT_FAIL=-1,
  };
//+------------------------------------------------------------------+
//| IPC copy ticks flags                                             |
//+------------------------------------------------------------------+
enum EnMT5IPCCopyTicksFlag : INT32
  {
   MT5_IPC_COPY_TICKS_ALL=-1,                  // everything
   MT5_IPC_COPY_TICKS_INFO=1,                  // bid ask
   MT5_IPC_COPY_TICKS_TRADE=2                  // last
  };
//+------------------------------------------------------------------+
//| IPC commands                                                     |
//+------------------------------------------------------------------+
enum EnMT5IPCCommand : UINT32
  {
   MT5_IPC_CMD_VERSION            =1,
   MT5_IPC_CMD_PING               =2,
   MT5_IPC_CMD_PONG               =3,
   MT5_IPC_CMD_INITIALIZE         =4,
   MT5_IPC_CMD_COPY_TICKS_FROM    =104,   // request N ticks from some date (see MQL5 CopyTicks)
   MT5_IPC_CMD_COPY_TICKS_RANGE   =105,   // request ticks from date range (see MQL5 CopyTicksRange)
   MT5_IPC_CMD_COPY_RATES_FROM    =106,   // request N rates form some date (see MQL5 CopyRates)
   MT5_IPC_CMD_COPY_RATES_RANGE   =107,   // request rates from date range (see MQL5 CopyRates)
   MT5_IPC_CMD_COPY_RATES_FROM_POS=108,   // request N rates form some position (see MQL5 CopyRates)
   MT5_IPC_CMD_TERMINAL_INFO      =109,   // get terminal info structure
  };
//+------------------------------------------------------------------+
//| MT5 periods                                                      |
//+------------------------------------------------------------------+
enum EnMT5TimeFame
  {
   MT5_TIMEFAME_M1  =1,
   MT5_TIMEFAME_M2  =2,
   MT5_TIMEFAME_M3  =3,
   MT5_TIMEFAME_M4  =4,
   MT5_TIMEFAME_M5  =5,
   MT5_TIMEFAME_M6  =6,
   MT5_TIMEFAME_M10 =10,
   MT5_TIMEFAME_M12 =12,
   MT5_TIMEFAME_M15 =15,
   MT5_TIMEFAME_M20 =20,
   MT5_TIMEFAME_M30 =30,
   MT5_TIMEFAME_H1  =1  | 0x4000,
   MT5_TIMEFAME_H2  =2  | 0x4000,
   MT5_TIMEFAME_H3  =3  | 0x4000,
   MT5_TIMEFAME_H4  =4  | 0x4000,
   MT5_TIMEFAME_H6  =6  | 0x4000,
   MT5_TIMEFAME_H8  =8  | 0x4000,
   MT5_TIMEFAME_H12 =12 | 0x4000,
   MT5_TIMEFAME_D1  =24 | 0x4000,
   MT5_TIMEFAME_W1  =1 | 0x8000,
   MT5_TIMEFAME_MON1=1 | 0xC000
  };
//+------------------------------------------------------------------+
//| MT5 Terminal connection statuses                                 |
//+------------------------------------------------------------------+
enum EnMT5ConnectionStatus
  {
   MT5_STATUS_DISCONNECTED=0,
   MT5_STATUS_CONNECTING  =1,
   MT5_STATUS_SYNCHRONIZED=2
  };
//+------------------------------------------------------------------+
//| Tick data structure                                              |
//+------------------------------------------------------------------+
#pragma pack(push,1)
struct MT5IPCTick
  {
   INT64             time;             // last time (unix time in sec)
   //---
   double            bid;              // bid
   double            ask;              // ask
   double            last;             // last
   //---
   UINT64            volume;           // volume
   INT64             time_msc;         // last time (unix time in msec)
   UINT32            flags;            // flags
   double            volume_dbl;       // volume
  };
#pragma pack(pop)
//+------------------------------------------------------------------+
//| Rate data structure                                              |
//+------------------------------------------------------------------+
#pragma pack(push,1)
struct MT5IPCRate
  {
   INT64             time;              // date and time
   //---
   double            open;              // open price (absolute value)
   double            high;              // max. price
   double            low;               // min. price
   double            close;             // close price
   //---
   UINT64            tick_volume;       // tick volume
   INT32             spread;            // spread
   UINT64            real_volume;       // real volume
  };
#pragma pack(pop)
//+------------------------------------------------------------------+
//| Rate data structure                                              |
//+------------------------------------------------------------------+
#pragma pack(push,1)
struct MT5IPCTerminalInfo
  {
   BYTE              status;             // connection status
   UINT64            login;              // login number
   wchar_t           server[128];        // server name
  };
#pragma pack(pop)
//+------------------------------------------------------------------+
//| Helper functions                                                 |
//+------------------------------------------------------------------+
inline bool IsMT5CmdFailed(EnMT5IPCResult res) { return(res < 0); }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
inline bool IsMT5CmdSucceeded(EnMT5IPCResult res) { return(res > 0); }
//+------------------------------------------------------------------+
