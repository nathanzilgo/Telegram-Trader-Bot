//+------------------------------------------------------------------+
//|                                                     MetaTrader 5 |
//|                   Copyright 2000-2019, MetaQuotes Software Corp. |
//|                                        http://www.metaquotes.net |
//+------------------------------------------------------------------+
#include "stdafx.h"
#include "MT5Connector.h"

//+------------------------------------------------------------------+
//| Helper allocator                                                 |
//+------------------------------------------------------------------+
struct MyAllocator
  {
   void* operator()(size_t size) const
     {
      return(new (std::nothrow) char[size]);
     }
  };
using MyStringMapper=UberMapper<wchar_t, MyAllocator>;
using MyTickMapper=UberMapper<MT5IPCTick, MyAllocator>;
using MyRateMapper=UberMapper<MT5IPCRate, MyAllocator>;
//+------------------------------------------------------------------+
//| Constructor                                                      |
//+------------------------------------------------------------------+
CMT5Connector::CMT5Connector(void)
  {
  }
//+------------------------------------------------------------------+
//| Destructor                                                       |
//+------------------------------------------------------------------+
CMT5Connector::~CMT5Connector(void)
  {
   Shutdown();
  }
//+------------------------------------------------------------------+
//| Initializing link with terminal                                  |
//+------------------------------------------------------------------+
bool CMT5Connector::Initialize(const wchar_t *path, const wchar_t *client_name)
  {
   if(!m_link.Initialize(path))
      return(false);
//--- first request
   if(!SendVars(MT5_IPC_CMD_INITIALIZE, UINT32(MT5_IPC_CURRENT_VERSION), UberString(client_name)))
      return(false);
   EnMT5IPCCommand cmd=EnMT5IPCCommand(0);
   EnMT5IPCResult status=EnMT5IPCResult(0);
   if(!RecvVars(&cmd, &status))
      return(false);
//--- check answer
   if(cmd!=MT5_IPC_CMD_INITIALIZE || IsMT5CmdFailed(status))
      return(false);
//---
   return(true);
  }
//+------------------------------------------------------------------+
//| Shutdowning link with terminal                                   |
//+------------------------------------------------------------------+
void CMT5Connector::Shutdown(void)
  {
   m_link.Shutdown();
  }
//+------------------------------------------------------------------+
//| Getting version                                                  |
//+------------------------------------------------------------------+
bool CMT5Connector::Version(UINT32 *version, UINT32 *build, char *build_date, UINT32 build_date_limit)
  {
//--- send request
   if(!SendVars(MT5_IPC_CMD_VERSION, UINT32(0)))
      return(false);
//--- receive response
   UINT32 res_version, res_build, build_date_len;
   wchar_t *res_build_date=nullptr;
   EnMT5IPCCommand cmd=EnMT5IPCCommand(0);
   EnMT5IPCResult status=EnMT5IPCResult(0);
   if(!RecvVars(&cmd, &status, &res_version, &res_build, MyStringMapper(&res_build_date, &build_date_len)))
      return(false);
//--- check answer
   if(cmd!=MT5_IPC_CMD_VERSION || IsMT5CmdFailed(status))
      return(false);
//--- save results
   if(version)
      *version=res_version;
   if(build)
      *build=res_build;

   if(build_date && build_date_limit > 0)
     {
      int used=0;
      if(res_build_date)
         used=WideCharToMultiByte(CP_UTF8, 0, res_build_date, int(build_date_len), build_date, int(build_date_limit) - 1, nullptr, nullptr);
      if(used>=0)
         build_date[used]='\0';
     }
//--- free dynamic memory
   if(res_build_date)
      delete[] res_build_date;
//--- success
   return(true);
  }
//+------------------------------------------------------------------+
//| Get ticks from terminal                                          |
//| dates in milliseconds                                            |
//+------------------------------------------------------------------+
bool CMT5Connector::CopyTicksFrom(const wchar_t *symbol_name, INT64 date_from, UINT32 count, UINT32 flags, MT5IPCTick **ticks, uint32_t *ticks_count)
  {
   if(ticks==nullptr || ticks_count==nullptr)
      return(false);
//--- send request
   if(!SendVars(MT5_IPC_CMD_COPY_TICKS_FROM, UberString(symbol_name), date_from, count, flags))
      return(false);
//--- receive response
   EnMT5IPCCommand cmd=EnMT5IPCCommand(0);
   EnMT5IPCResult status=EnMT5IPCResult(0);
   if(!RecvVars(&cmd, &status, MyTickMapper(ticks, ticks_count)))
      return(false);
//--- check answer
   if(cmd!=MT5_IPC_CMD_COPY_TICKS_FROM || IsMT5CmdFailed(status))
      return(false);
//--- 
   return(true);
  }
//+------------------------------------------------------------------+
//| Get ticks from terminal                                          |
//| dates in milliseconds                                            |
//+------------------------------------------------------------------+
bool CMT5Connector::CopyTicksRange(const wchar_t *symbol_name, INT64 date_from, INT64 date_to, UINT32 flags, MT5IPCTick **ticks, uint32_t *ticks_count)
  {
   if(ticks==nullptr || ticks_count==nullptr)
      return(false);
//--- send request
   if(!SendVars(MT5_IPC_CMD_COPY_TICKS_RANGE, UberString(symbol_name), date_from, date_to, flags))
      return(false);
//--- receive response
   EnMT5IPCCommand cmd=EnMT5IPCCommand(0);
   EnMT5IPCResult status=EnMT5IPCResult(0);
   if(!RecvVars(&cmd, &status, MyTickMapper(ticks, ticks_count)))
      return(false);
//--- check answer
   if(cmd!=MT5_IPC_CMD_COPY_TICKS_RANGE || IsMT5CmdFailed(status))
      return(false);
//--- 
   return(true);
  }
//+------------------------------------------------------------------+
//| Freeing memory for ticks                                         |
//+------------------------------------------------------------------+
void CMT5Connector::FreeTicks(MT5IPCTick *ticks)
  {
   delete[](char*)ticks;
  }
//+------------------------------------------------------------------+
//| Get rates from terminal                                          |
//| dates in seconds                                                 |
//+------------------------------------------------------------------+
bool CMT5Connector::CopyRatesFrom(const wchar_t *symbol_name, UINT32 timeframe, INT64 date_from, UINT32 count, MT5IPCRate **rates, uint32_t *rates_count)
  {
   if(rates==nullptr || rates_count==nullptr)
      return(false);
//--- send request
   if(!SendVars(MT5_IPC_CMD_COPY_RATES_FROM, UberString(symbol_name), timeframe, date_from, count))
      return(false);
//--- receive response
   EnMT5IPCCommand cmd=EnMT5IPCCommand(0);
   EnMT5IPCResult status=EnMT5IPCResult(0);
   if(!RecvVars(&cmd, &status, MyRateMapper(rates, rates_count)))
      return(false);
//--- check answer
   if(cmd!=MT5_IPC_CMD_COPY_RATES_FROM || IsMT5CmdFailed(status))
      return(false);
//--- 
   return(true);
  }
//+------------------------------------------------------------------+
//| Get rates from terminal                                          |
//+------------------------------------------------------------------+
bool CMT5Connector::CopyRatesFromPos(const wchar_t *symbol_name, UINT32 timeframe, UINT32 start_pos, UINT32 count, MT5IPCRate **rates, uint32_t *rates_count)
  {
   if(rates==nullptr || rates_count==nullptr)
      return(false);
//--- send request
   if(!SendVars(MT5_IPC_CMD_COPY_RATES_FROM_POS, UberString(symbol_name), timeframe, start_pos, count))
      return(false);
//--- receive response
   EnMT5IPCCommand cmd=EnMT5IPCCommand(0);
   EnMT5IPCResult status=EnMT5IPCResult(0);
   if(!RecvVars(&cmd, &status, MyRateMapper(rates, rates_count)))
      return(false);
//--- check answer
   if(cmd!=MT5_IPC_CMD_COPY_RATES_FROM_POS || IsMT5CmdFailed(status))
      return(false);
//--- 
   return(true);
  }
//+------------------------------------------------------------------+
//| Get rates from terminal                                          |
//| dates in seconds                                                 |
//+------------------------------------------------------------------+
bool CMT5Connector::CopyRatesFromRange(const wchar_t *symbol_name, UINT32 timeframe, INT64 date_from, INT64 date_to, MT5IPCRate **rates, uint32_t *rates_count)
  {
   if(rates==nullptr || rates_count==nullptr)
      return(false);
//--- send request
   if(!SendVars(MT5_IPC_CMD_COPY_RATES_RANGE, UberString(symbol_name), timeframe, date_from, date_to))
      return(false);
//--- receive response
   EnMT5IPCCommand cmd=EnMT5IPCCommand(0);
   EnMT5IPCResult status=EnMT5IPCResult(0);
   if(!RecvVars(&cmd, &status, MyRateMapper(rates, rates_count)))
      return(false);
//--- check answer
   if(cmd!=MT5_IPC_CMD_COPY_RATES_RANGE || IsMT5CmdFailed(status))
      return(false);
//--- 
   return(true);
  }
//+------------------------------------------------------------------+
//| Freeing memory for rates                                         |
//+------------------------------------------------------------------+
void CMT5Connector::FreeRates(MT5IPCRate *rates)
  {
   delete[](char*)rates;
  }
//+------------------------------------------------------------------+
//| Terminal status                                                  |
//+------------------------------------------------------------------+
bool CMT5Connector::TerminalInfo(MT5IPCTerminalInfo *info)
  {
   if(info==nullptr)
      return(false);
//--- send request
   if(!SendVars(MT5_IPC_CMD_TERMINAL_INFO))
      return(false);
//--- receive response
   EnMT5IPCCommand cmd=EnMT5IPCCommand(0);
   EnMT5IPCResult status=EnMT5IPCResult(0);
   if(!RecvVars(&cmd, &status, info))
      return(false);
//--- check answer
   if(cmd!=MT5_IPC_CMD_TERMINAL_INFO || IsMT5CmdFailed(status))
      return(false);
//--- 
   return(true);
  }
//+------------------------------------------------------------------+
