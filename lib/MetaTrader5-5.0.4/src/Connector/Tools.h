//+------------------------------------------------------------------+
//|                                                     MetaTrader 5 |
//|                   Copyright 2000-2019, MetaQuotes Software Corp. |
//|                                        http://www.metaquotes.net |
//+------------------------------------------------------------------+
#pragma once
//+------------------------------------------------------------------+
//| Helper class                                                     |
//| Stack based conversion UTF8 -> UTF16                             |
//| If not fit allocated dynamically                                 |
//+------------------------------------------------------------------+
template <size_t t_stack_len=100>
class UTF8To16
  {
public:
                     UTF8To16(const char *text) : UTF8To16(text, text ? strlen(text) : 0) {}
   UTF8To16(const char *text, size_t text_len)
     {
      if(text==nullptr)
         return;
      //---
      int out_len, need;
      if((need=MultiByteToWideChar(CP_UTF8, 0, text, int(text_len), nullptr, 0))<=0)
         return;
      //---
      if(size_t(need + 1) > _countof(m_local))
        {
         wchar_t *new_text=new(std::nothrow) wchar_t[need + 1];
         if(new_text==nullptr)
            return;
         m_text=new_text;
         out_len=need + 1;
        }
      else out_len=_countof(m_local) - 1;
      //---
      if(MultiByteToWideChar(CP_UTF8, 0, text, int(text_len), m_text, out_len)!=need)
         m_text[0]=L'\0';
      else m_text[need]=L'\0';
     }
   ~UTF8To16()
     {
      if(m_text!=m_local)
        {
         delete[] m_text;
         m_text=m_local;
        }
     }

   const wchar_t*    get() const { return m_text; }
   operator const    wchar_t*() const { return m_text; }

private:
   wchar_t           m_local[t_stack_len]={};
   wchar_t          *m_text=m_local;

   UTF8To16&         operator=(const UTF8To16&)=delete;
                     UTF8To16(const UTF8To16&)=delete;
  };
//+------------------------------------------------------------------+
//| Helper class                                                     |
//| Stack based conversion UTF8 -> UTF16                             |
//| If not fit allocated dynamically                                 |
//+------------------------------------------------------------------+
template <size_t t_stack_len=100>
class UTF16To8
  {
public:
                     UTF16To8(const wchar_t *text) : UTF16To8(text, text ? wcslen(text) : 0) {}
   UTF16To8(const wchar_t *text, size_t text_len)
     {
      if(text==nullptr)
         return;
      //---
      int out_len, need;
      if((need=WideCharToMultiByte(CP_UTF8, 0, text, int(text_len), nullptr, 0, 0, 0))<=0)
         return;
      //---
      if(size_t(need + 1) > _countof(m_local))
        {
         char *new_text=new(std::nothrow) char[need + 1];
         if(new_text==nullptr)
            return;
         m_text=new_text;
         out_len=need + 1;
        }
      else out_len=_countof(m_local) - 1;
      //---
      if(WideCharToMultiByte(CP_UTF8, 0, text, int(text_len), m_text, out_len, 0, 0)!=need)
         m_text[0]=L'\0';
      else m_text[need]=L'\0';
     }
   ~UTF16To8()
     {
      if(m_text!=m_local)
        {
         delete[] m_text;
         m_text=m_local;
        }
     }

   const char*       get() const { return m_text; }
   operator const    char*() const { return m_text; }

private:
   char              m_local[t_stack_len]={};
   char             *m_text=m_local;

   UTF16To8&         operator=(const UTF16To8&)=delete;
                     UTF16To8(const UTF16To8&)=delete;
  };
//--- helpers
using ToUTF16=UTF8To16<>;
using ToUTF8=UTF16To8<>;
//+------------------------------------------------------------------+
