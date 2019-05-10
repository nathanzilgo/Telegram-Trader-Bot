//+------------------------------------------------------------------+
//|                                                     MetaTrader 5 |
//|                   Copyright 2000-2019, MetaQuotes Software Corp. |
//|                                        http://www.metaquotes.net |
//+------------------------------------------------------------------+
#pragma once
#include "..\Interprocess\InterClient.h"
#include "..\Interprocess\InterDefs.h"
//+------------------------------------------------------------------+
//| Base MetaTrader5 Connector                                       |
//+------------------------------------------------------------------+
class CMT5Connector
  {
private:
   CInterClient      m_link;

public:
                     CMT5Connector(void);
                    ~CMT5Connector(void);

   bool              Initialize(const wchar_t *path, const wchar_t *client_name);
   void              Shutdown(void);

   bool              Version(UINT32 *version, UINT32 *build, char *build_date, UINT32 build_date_limit);

   bool              CopyTicksFrom(const wchar_t *symbol_name, INT64 date_from, UINT32 count, UINT32 flags, MT5IPCTick **ticks, uint32_t *ticks_count);
   bool              CopyTicksRange(const wchar_t *symbol_name, INT64 date_from, INT64 date_to, UINT32 flags, MT5IPCTick **ticks, uint32_t *ticks_count);
   void              FreeTicks(MT5IPCTick *ticks);

   bool              CopyRatesFrom(const wchar_t *symbol_name, UINT32 timeframe, INT64 date_from, UINT32 count, MT5IPCRate **rates, uint32_t *rates_count);
   bool              CopyRatesFromPos(const wchar_t *symbol_name, UINT32 timeframe, UINT32 start_pos, UINT32 count, MT5IPCRate **rates, uint32_t *rates_count);
   bool              CopyRatesFromRange(const wchar_t *symbol_name, UINT32 timeframe, INT64 date_from, INT64 date_to, MT5IPCRate **rates, uint32_t *rates_count);
   void              FreeRates(MT5IPCRate *rates);

   bool              IsInitialized() { return(m_link.IsValid()); }

   bool              TerminalInfo(MT5IPCTerminalInfo *info);

private:
   //--- forbidden
                     CMT5Connector(const CMT5Connector&)=delete;
   bool              operator=(const CMT5Connector&)=delete;

   template <typename... TArgs> bool SendVars(TArgs&&... args) { return(m_link.SendVars(std::forward<TArgs>(args)...)); }
   template <typename... TArgs> bool RecvVars(TArgs&&... args) { return(m_link.RecvVars(std::forward<TArgs>(args)...)); }
  };
//+------------------------------------------------------------------+
