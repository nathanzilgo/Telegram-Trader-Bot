//+------------------------------------------------------------------+
//|                                                     MetaTrader 5 |
//|                   Copyright 2000-2019, MetaQuotes Software Corp. |
//|                                        http://www.metaquotes.net |
//+------------------------------------------------------------------+
#include "stdafx.h"
#include "Connector\MT5Connector.h"
#include "Connector\Tools.h"

//--- constants
enum
  {
   TERMINAL_WAIT_TIMEOUT=60 * 1000 // Terminal wait timeout
  };
//--- our main global connection
CMT5Connector ExtMT5Connector;
//--- our Python types
PyTypeObject ExtPyTickType={}, ExtPyRateType={};
//+------------------------------------------------------------------+
//| Start MetaTrader 5 Terminal                                      |
//+------------------------------------------------------------------+
PyObject* Py_MT5Initialize(PyObject* /*self*/, PyObject *args)
  {
   bool bres=false;
   ExtMT5Connector.Shutdown();
//---
   if(PyTuple_Size(args)==0)
     {
      //--- initialize connection
      bres=ExtMT5Connector.Initialize(nullptr, L"Python");
     }
   else
     {
      const char *path;
      //--- check types
      if(PyTuple_Size(args)!=1 || !PyArg_ParseTuple(args, "s", &path) || path==nullptr)
        {
         PyErr_SetString(PyExc_RuntimeError, "Invalid arguments");
         return(nullptr);
        }
      //--- initialize connection
      bres=ExtMT5Connector.Initialize(ToUTF16(path), L"Python");
     }
//---
   return(bres ? (Py_INCREF(Py_True), Py_True) : (Py_INCREF(Py_False), Py_False));
  }
//+------------------------------------------------------------------+
//| Disconnect from Terminal                                         |
//+------------------------------------------------------------------+
PyObject* Py_MT5Shutdown(PyObject* /*self*/, PyObject* /*args*/)
  {
   ExtMT5Connector.Shutdown();
//---
   return(Py_INCREF(Py_True), Py_True);
  }
//+------------------------------------------------------------------+
//| Get Terminal version                                             |
//+------------------------------------------------------------------+
PyObject* Py_MT5Version(PyObject* /*self*/, PyObject* /*args*/)
  {
//--- check connection
   if(!ExtMT5Connector.IsInitialized())
     {
      PyErr_SetString(PyExc_RuntimeError, "No IPC connection");
      return(nullptr);
     }
//--- call version
   uint32_t version, build;
   char build_date[128]={};
   if(!ExtMT5Connector.Version(&version, &build, build_date, _countof(build_date)))
     {
      PyErr_SetString(PyExc_RuntimeError, "IPC call failed");
      return(nullptr);
     }
//--- return list
   return(Py_BuildValue("[iis]", version, build, build_date));
  }
//+------------------------------------------------------------------+
//| Get Terminal status                                              |
//+------------------------------------------------------------------+
PyObject* Py_MT5TerminalInfo(PyObject* /*self*/, PyObject* /*args*/)
  {
//--- check connection
   if(!ExtMT5Connector.IsInitialized())
     {
      PyErr_SetString(PyExc_RuntimeError, "No IPC connection");
      return(nullptr);
     }
//--- call version
   MT5IPCTerminalInfo info={};
   if(!ExtMT5Connector.TerminalInfo(&info))
     {
      PyErr_SetString(PyExc_RuntimeError, "IPC call failed");
      return(nullptr);
     }

   char text[200]={};
   sprintf_s(text, "%I64u", info.login);
//--- return list
   return(Py_BuildValue("[iss]", info.status, ToUTF8(info.server).get(), text));
  }
//+------------------------------------------------------------------+
//| Wait for Terminal start                                          |
//+------------------------------------------------------------------+
PyObject* Py_MT5WaitForTerminal(PyObject* /*self*/, PyObject* /*args*/)
  {
//--- check connection
   if(!ExtMT5Connector.IsInitialized())
     {
      PyErr_SetString(PyExc_RuntimeError, "No IPC connection");
      return(nullptr);
     }
//--- call version
   MT5IPCTerminalInfo info={};

   for(UINT32 tick=0; tick < TERMINAL_WAIT_TIMEOUT; tick+=100, Sleep(100))
     {
      if(!ExtMT5Connector.TerminalInfo(&info))
        {
         PyErr_SetString(PyExc_RuntimeError, "IPC call failed");
         return(nullptr);
        }
      if(info.status==MT5_STATUS_SYNCHRONIZED)
         return(Py_INCREF(Py_True), Py_True);
     }
//--- nothing connected
   return(Py_INCREF(Py_False), Py_False);
  }
//+------------------------------------------------------------------+
//| Build R matrix variable from data                                |
//+------------------------------------------------------------------+
PyObject* BuildTicksMatrix(MT5IPCTick *ticks, UINT32 ticks_count)
  {
//--- check
   if(ticks==nullptr)
      return(nullptr);
//--- set results
   PyObject* py_result=PyTuple_New(ticks_count);
   if(py_result)
     {
      for(UINT32 idx=0; idx < ticks_count; idx++)
        {
         PyObject* py_tuple=PyStructSequence_New(&ExtPyTickType);
         if(py_tuple==nullptr)
            return(nullptr);
         //--- setting datetime
         PyObject* py_datetime=nullptr;
         if(PyObject* py_temp=PyFloat_FromDouble(ticks[idx].time_msc/1000.))
           {
            if(PyObject* py_args=PyTuple_New(1))
              {
               PyTuple_SetItem(py_args, 0, py_temp);
               py_datetime=PyDateTime_FromTimestamp(py_args);
               Py_DECREF(py_args);
              }
            else
               Py_DECREF(py_temp);
           }

         PyStructSequence_SetItem(py_tuple, 0, py_datetime ? py_datetime : (Py_INCREF(Py_None), Py_None));
         PyStructSequence_SetItem(py_tuple, 1, PyFloat_FromDouble(ticks[idx].bid));
         PyStructSequence_SetItem(py_tuple, 2, PyFloat_FromDouble(ticks[idx].ask));
         PyStructSequence_SetItem(py_tuple, 3, PyFloat_FromDouble(ticks[idx].last));
         PyStructSequence_SetItem(py_tuple, 4, PyFloat_FromDouble(ticks[idx].volume_dbl));
         PyStructSequence_SetItem(py_tuple, 5, PyLong_FromLong(ticks[idx].flags));

         PyTuple_SetItem(py_result, idx, py_tuple);
        }
     }
//--- return result
   return(py_result);
  }
//+------------------------------------------------------------------+
//| Build R matrix variable from data                                |
//+------------------------------------------------------------------+
PyObject* BuildRatesMatrix(MT5IPCRate *rates, UINT32 rates_count)
  {
//--- check
   if(rates==nullptr)
      return(nullptr);
//--- set results
   PyObject* py_result=PyTuple_New(rates_count);
   if(py_result)
     {
      for(UINT32 idx=0; idx < rates_count; idx++)
        {
         PyObject* py_tuple=PyStructSequence_New(&ExtPyRateType);
         if(py_tuple==nullptr)
            return(nullptr);
         //--- setting datetime
         PyObject* py_datetime=nullptr;
         if(PyObject* py_temp=PyLong_FromLongLong(rates[idx].time))
           {
            if(PyObject* py_args=PyTuple_New(1))
              {
               PyTuple_SetItem(py_args, 0, py_temp);
               py_datetime=PyDateTime_FromTimestamp(py_args);
               Py_DECREF(py_args);
              }
            else
               Py_DECREF(py_temp);
           }

         PyStructSequence_SetItem(py_tuple, 0, py_datetime ? py_datetime : (Py_INCREF(Py_None), Py_None));
         PyStructSequence_SetItem(py_tuple, 1, PyFloat_FromDouble(rates[idx].open));
         PyStructSequence_SetItem(py_tuple, 2, PyFloat_FromDouble(rates[idx].high));
         PyStructSequence_SetItem(py_tuple, 3, PyFloat_FromDouble(rates[idx].low));
         PyStructSequence_SetItem(py_tuple, 4, PyFloat_FromDouble(rates[idx].close));
         PyStructSequence_SetItem(py_tuple, 5, PyLong_FromUnsignedLongLong(rates[idx].tick_volume));
         PyStructSequence_SetItem(py_tuple, 6, PyLong_FromLong(rates[idx].spread));
         PyStructSequence_SetItem(py_tuple, 7, PyLong_FromUnsignedLongLong(rates[idx].real_volume));

         PyTuple_SetItem(py_result, idx, py_tuple);
        }
     }
//--- return result
   return(py_result);
  }
//+------------------------------------------------------------------+
//| Get unixtime from python datetime/integer/float                  |
//+------------------------------------------------------------------+
bool ParsePythonDateTime(PyObject* py_obj, int *res)
  {
//--- checks
   if(py_obj==nullptr)
      return(false);

   int tmp=0;
//--- convertings
   if(PyLong_Check(py_obj))
      tmp=PyLong_AsLong(py_obj);
   else
      if(PyFloat_Check(py_obj))
         tmp=(int)PyFloat_AsDouble(py_obj);
      else
         if(PyDateTime_Check(py_obj))
           {
            if(PyObject *py_res=PyObject_CallMethod(py_obj, "timestamp", ""))
              {
               tmp=PyLong_AsLong(py_res);
               Py_DECREF(py_res);
              }
            else
               return(false);
           }
         else
            return(false);
//--- store if needed
   if(tmp)
      *res=tmp;
//--- success
   return(true);
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
PyObject* Py_MT5CopyTicksFrom(PyObject* /*self*/, PyObject* args)
  {
//--- check connection
   if(!ExtMT5Connector.IsInitialized())
     {
      PyErr_SetString(PyExc_RuntimeError, "No IPC connection");
      return(nullptr);
     }

   const char *val_symbol=nullptr;
   PyObject *py_from=nullptr;
   int val_from, val_count, val_flags;
//--- check types
   if(PyTuple_Size(args)!=4 || !PyArg_ParseTuple(args, "sOii", &val_symbol, &py_from, &val_count, &val_flags) || val_symbol==nullptr || !ParsePythonDateTime(py_from, &val_from))
     {
      PyErr_SetString(PyExc_RuntimeError, "Invalid arguments");
      return(nullptr);
     }
//---
   MT5IPCTick *ticks=nullptr;
   UINT32      ticks_count=0;
//--- calling native copy ticks
   if(!ExtMT5Connector.CopyTicksFrom(ToUTF16(val_symbol), val_from * 1000LL, val_count, val_flags, &ticks, &ticks_count))
     {
      PyErr_SetString(PyExc_RuntimeError, "IPC call failed");
      return(nullptr);
     }
//--- call internal mapper
   PyObject *py_result=BuildTicksMatrix(ticks, ticks_count);
//--- free ticks memory
   ExtMT5Connector.FreeTicks(ticks);
//--- check for error
   if(py_result==nullptr)
     {
      PyErr_SetString(PyExc_RuntimeError, "No memory");
      return(nullptr);
     }
//--- success
   return(py_result);
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
PyObject* Py_MT5CopyTicksRange(PyObject* /*self*/, PyObject* args)
  {
//--- check connection
   if(!ExtMT5Connector.IsInitialized())
     {
      PyErr_SetString(PyExc_RuntimeError, "No IPC connection");
      return(nullptr);
     }

   const char *val_symbol;
   PyObject *py_from=nullptr, *py_to=nullptr;
   int val_from, val_to, val_flags;
//--- check types
   if(PyTuple_Size(args)!=4 || !PyArg_ParseTuple(args, "sOOi", &val_symbol, &py_from, &py_to, &val_flags) || val_symbol==nullptr || !ParsePythonDateTime(py_from, &val_from) || !ParsePythonDateTime(py_to, &val_to))
     {
      PyErr_SetString(PyExc_RuntimeError, "Invalid arguments");
      return(nullptr);
     }

   MT5IPCTick *ticks=nullptr;
   UINT32      ticks_count=0;
//--- calling native copy ticks
   if(!ExtMT5Connector.CopyTicksRange(ToUTF16(val_symbol), val_from * 1000LL, val_to * 1000LL, val_flags, &ticks, &ticks_count))
     {
      PyErr_SetString(PyExc_RuntimeError, "IPC call failed");
      return(nullptr);
     }
//--- call internal mapper
   PyObject *py_result=BuildTicksMatrix(ticks, ticks_count);
//--- free ticks memory
   ExtMT5Connector.FreeTicks(ticks);
//--- check for error
   if(py_result==nullptr)
     {
      PyErr_SetString(PyExc_RuntimeError, "No memory");
      return(nullptr);
     }
//--- success
   return(py_result);
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
PyObject* Py_MT5CopyRatesFrom(PyObject* /*self*/, PyObject* args)
  {
//--- check connection
   if(!ExtMT5Connector.IsInitialized())
     {
      PyErr_SetString(PyExc_RuntimeError, "No IPC connection");
      return(nullptr);
     }

   const char *val_symbol;
   PyObject* py_from=nullptr;
   int val_timefame, val_from, val_count;
//--- check types
   if(PyTuple_Size(args)!=4 || !PyArg_ParseTuple(args, "siOi", &val_symbol, &val_timefame, &py_from, &val_count) || val_symbol==nullptr || !ParsePythonDateTime(py_from, &val_from))
     {
      PyErr_SetString(PyExc_RuntimeError, "Invalid arguments");
      return(nullptr);
     }

   MT5IPCRate *rates=nullptr;
   UINT32      rates_count=0;
//--- calling native copy rates
   if(!ExtMT5Connector.CopyRatesFrom(ToUTF16(val_symbol), val_timefame, val_from, val_count, &rates, &rates_count))
     {
      PyErr_SetString(PyExc_RuntimeError, "IPC call failed");
      return(nullptr);
     }
//--- call internal mapper
   PyObject *py_result=BuildRatesMatrix(rates, rates_count);
//--- free rates memory
   ExtMT5Connector.FreeRates(rates);
//--- check for error
   if(py_result==nullptr)
     {
      PyErr_SetString(PyExc_RuntimeError, "No memory");
      return(nullptr);
     }
//--- success
   return(py_result);
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
PyObject* Py_MT5CopyRatesFromPos(PyObject* /*self*/, PyObject* args)
  {
//--- check connection
   if(!ExtMT5Connector.IsInitialized())
     {
      PyErr_SetString(PyExc_RuntimeError, "No IPC connection");
      return(nullptr);
     }

   const char *val_symbol;
   int val_timefame, val_start_pos, val_count;
//--- check types
   if(PyTuple_Size(args)!=4 || !PyArg_ParseTuple(args, "siii", &val_symbol, &val_timefame, &val_start_pos, &val_count) || val_symbol==nullptr)
     {
      PyErr_SetString(PyExc_RuntimeError, "Invalid arguments");
      return(nullptr);
     }

   MT5IPCRate *rates=nullptr;
   UINT32      rates_count=0;
//--- calling native copy rates
   if(!ExtMT5Connector.CopyRatesFromPos(ToUTF16(val_symbol), val_timefame, val_start_pos, val_count, &rates, &rates_count))
     {
      PyErr_SetString(PyExc_RuntimeError, "IPC call failed");
      return(nullptr);
     }
//--- call internal mapper
   PyObject *py_result=BuildRatesMatrix(rates, rates_count);
//--- free rates memory
   ExtMT5Connector.FreeRates(rates);
//--- check for error
   if(py_result==nullptr)
     {
      PyErr_SetString(PyExc_RuntimeError, "No memory");
      return(nullptr);
     }
//--- success
   return(py_result);
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
PyObject* Py_MT5CopyRatesRange(PyObject* /*self*/, PyObject* args)
  {
//--- check connection
   if(!ExtMT5Connector.IsInitialized())
     {
      PyErr_SetString(PyExc_RuntimeError, "No IPC connection");
      return(nullptr);
     }

   const char *val_symbol;
   PyObject *py_from=nullptr, *py_to=nullptr;
   int val_timefame, val_from, val_to;
//--- check types
   if(PyTuple_Size(args)!=4 || !PyArg_ParseTuple(args, "siOO", &val_symbol, &val_timefame, &py_from, &py_to) || val_symbol==nullptr || !ParsePythonDateTime(py_from, &val_from) || !ParsePythonDateTime(py_to, &val_to))
     {
      PyErr_SetString(PyExc_RuntimeError, "Invalid arguments");
      return(nullptr);
     }

   MT5IPCRate *rates=nullptr;
   UINT32      rates_count=0;
//--- calling native copy rates
   if(!ExtMT5Connector.CopyRatesFromRange(ToUTF16(val_symbol), val_timefame, val_from, val_to, &rates, &rates_count))
     {
      PyErr_SetString(PyExc_RuntimeError, "IPC call failed");
      return(nullptr);
     }
//--- call internal mapper
   PyObject *py_result=BuildRatesMatrix(rates, rates_count);
//--- free rates memory
   ExtMT5Connector.FreeRates(rates);
//--- check for error
   if(py_result==nullptr)
     {
      PyErr_SetString(PyExc_RuntimeError, "No memory");
      return(nullptr);
     }
//--- success
   return(py_result);
  }
//+------------------------------------------------------------------+
//| Registration routine                                             |
//+------------------------------------------------------------------+
PyMODINIT_FUNC PyInit_C(void)
  {
   //while(!IsDebuggerPresent())
   //   Sleep(100);
//--- our module's function definition struct
   static PyMethodDef my_methods[] =
   {
        { "MT5Initialize",     Py_MT5Initialize,       METH_VARARGS, "MT5Initialize(path=None)\n\nEstablish connection with the MetaTrader 5 Terminal" },
      { "MT5Shutdown",         Py_MT5Shutdown,         METH_NOARGS,  "MT5Shutdown()\n\nDisconnect from the MetaTrader 5 Terminal" },
      //---
      { "MT5Version",          Py_MT5Version,          METH_NOARGS,  "MT5Version()\n\nGet the MetaTrader 5 Terminal version" },
      { "MT5TerminalInfo",     Py_MT5TerminalInfo,     METH_NOARGS,  "MT5TerminalInfo()\n\nGet the state and parameters of the MetaTrader 5 terminal connection" },
      { "MT5WaitForTerminal",  Py_MT5WaitForTerminal,  METH_NOARGS,  "MT5WaitForTerminal()\n\nWait for the MetaTrader 5 Terminal to connect to a broker's server" },
      //---
      { "MT5CopyTicksFrom",    Py_MT5CopyTicksFrom,    METH_VARARGS, "MT5CopyTicksFrom(symbol, from, count, flags)\n\nGet ticks starting from the specific date" },
      { "MT5CopyTicksRange",   Py_MT5CopyTicksRange,   METH_VARARGS, "MT5CopyTicksRange(symbol, from, to, flags)\n\nGet ticks from the specified period" },
      { "MT5CopyRatesFrom",    Py_MT5CopyRatesFrom,    METH_VARARGS, "MT5CopyRatesFrom(symbol, timeframe, from, count)\n\nGet bars starting from the specific date" },
      { "MT5CopyRatesFromPos", Py_MT5CopyRatesFromPos, METH_VARARGS, "MT5CopyRatesFromPos(symbol, timeframe, start_pos, count)\n\nGet bars starting from the specified position" },
      { "MT5CopyRatesRange",   Py_MT5CopyRatesRange,   METH_VARARGS, "MT5CopyRatesRange(symbol, timeframe, date_from, date_to)\n\nGet bars from the specified period" },
      //---
      { nullptr, nullptr, 0, nullptr }
     };
//--- our module definition struct
   static PyModuleDef my_module =
   {
       PyModuleDef_HEAD_INIT,
       "MetaTrader5",
       "API Connector to MetaTrader 5 Terminal",
       -1,
       my_methods
      };
//--- init datetime libs
   PyDateTime_IMPORT;
//--- create our module
   PyObject *py_module=PyModule_Create(&my_module);
   if(py_module)
     {
      static PyStructSequence_Field tick_fields[] =
        {
         "time", nullptr,
         "bid", nullptr,
         "ask", nullptr,
         "last", nullptr,
         "volume", nullptr,
         "flags", nullptr,
         nullptr, nullptr
         };
      static PyStructSequence_Desc tick_desc={};
      tick_desc.name="MT5Tick";
      tick_desc.doc="MetaTrader5 ticks sequence";
      tick_desc.fields=tick_fields;
      tick_desc.n_in_sequence=_countof(tick_fields)-1;
      //--- init named sequence
      PyStructSequence_InitType(&ExtPyTickType, &tick_desc);
      Py_INCREF(&ExtPyTickType);
      //--- register new object
      PyModule_AddObject(py_module, "MT5Tick", (PyObject*)&ExtPyTickType);
      //--- next item
      static PyStructSequence_Field rate_fields[] =
        {
         "time", nullptr,
         "open", nullptr,
         "low", nullptr,
         "high", nullptr,
         "close", nullptr,
         "tick_volume", nullptr,
         "spread", nullptr,
         "real_volume", nullptr,
         nullptr, nullptr
         };
      static PyStructSequence_Desc rate_desc={};
      rate_desc.name="MT5Rate";
      rate_desc.doc="MetaTrader5 rates sequence";
      rate_desc.fields=rate_fields;
      rate_desc.n_in_sequence=_countof(rate_fields)-1;
      //--- init named sequence
      PyStructSequence_InitType(&ExtPyRateType, &rate_desc);
      Py_INCREF(&ExtPyRateType);
      //--- register new object
      PyModule_AddObject(py_module, "MT5Rate", (PyObject*)&ExtPyRateType);
     }
//--- create and register module
   return(py_module);
  }
  //+------------------------------------------------------------------+
//| Dll entry point                                                  |
//+------------------------------------------------------------------+
  BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
  {
//---
   switch(ul_reason_for_call)
     {
      case DLL_PROCESS_ATTACH:
         break;

      case DLL_THREAD_ATTACH:
         break;

      case DLL_THREAD_DETACH:
         break;

      case DLL_PROCESS_DETACH:
         ExtMT5Connector.Shutdown();
         break;
     }
//---
   return(TRUE);
  }
//+------------------------------------------------------------------+
