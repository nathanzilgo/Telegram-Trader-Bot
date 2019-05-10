//+------------------------------------------------------------------+
//|                                                     MetaTrader 5 |
//|                   Copyright 2000-2019, MetaQuotes Software Corp. |
//|                                        http://www.metaquotes.net |
//+------------------------------------------------------------------+
#pragma once

//--- Windows version
#define WINVER               _WIN32_WINNT_VISTA
#define _WIN32_WINNT         _WIN32_WINNT_VISTA
#define _WIN32_WINDOWS       _WIN32_WINNT_VISTA

#define WIN32_LEAN_AND_MEAN

//--- Windows header files
#include <sdkddkver.h>
#include <windows.h>
#include <wincrypt.h>
#include <strsafe.h>
#include <shlobj.h>

//--- C++ standart library
#include <algorithm>

//--- Python includes
#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
#include <datetime.h>
//+------------------------------------------------------------------+
