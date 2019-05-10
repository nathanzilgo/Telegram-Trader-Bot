//+------------------------------------------------------------------+
//|                                                     MetaTrader 5 |
//|                   Copyright 2000-2019, MetaQuotes Software Corp. |
//|                                        http://www.metaquotes.net |
//+------------------------------------------------------------------+
#pragma once
//+------------------------------------------------------------------+
//| Special block for size inserts                                   |
//+------------------------------------------------------------------+
enum EnBlockSizer { BlockSizer };
//+------------------------------------------------------------------+
//| Helper structure for adding strings via variadic                 |
//+------------------------------------------------------------------+
struct UberString
  {
   const wchar_t    *text=nullptr;
   const uint32_t    len=0;

                     UberString(const wchar_t *text_, const uint32_t len_) : text(text_), len(len_) {}
                     UberString(const wchar_t *text_) : text(text_), len(text_ ? uint32_t(wcslen(text_)) : 0) {}
  };
//+------------------------------------------------------------------+
//| Helper structure for adding arrays via variadic                  |
//+------------------------------------------------------------------+
template <typename T> struct UberArray
  {
   const T          *data=nullptr;
   const uint32_t    count=0;

                     UberArray(const T *data_, const uint32_t count_) : data(data_), count(count_) {}
  };
//+------------------------------------------------------------------+
//| Helper structure for getting arrays via variadic                 |
//+------------------------------------------------------------------+
template <typename T, typename TAllocator> struct UberMapper
  {
   T               **data=nullptr;
   uint32_t         *count=nullptr;

   TAllocator        allocator;

                     UberMapper(TAllocator allocator_, T **data_, uint32_t *count_=nullptr) : allocator(allocator_), data(data_), count(count_) {}
                     UberMapper(T **data_, uint32_t *count_=nullptr) : data(data_), count(count_) {}
  };
//+------------------------------------------------------------------+
//| Shared inteprocess communication class (client side)             |
//| message based, not stream based                                  |
//+------------------------------------------------------------------+
class CInterClient
  {
private:
   enum
     {
      PIPE_WAIT_TIMEOUT=60 * 1000,                        // pipe wait timeout таймаут на ожидание соединения
      STACK_BUF_SIZE=16 * 1024,                           // small stack buffer
      MAX_TERMINAL_LAUNCH_TIME=30*1000                    // max Terminal launch timeout
     };

   HANDLE            m_pipe=INVALID_HANDLE_VALUE;         // main pipe connection
   uint8_t           m_unique_hash[32]={};                // our unique hash name for pipe
   wchar_t           m_error_msg[2048]={};                // error string for logging

   //--- for generating error strings
   void SetErrorMsg(const wchar_t *msg)
     {
      StringCchCopy(m_error_msg, _countof(m_error_msg), msg);
     }
   template <typename... TArgs> void SetErrorMsg(const wchar_t *msg, TArgs&&... args)
     {
      StringCchPrintf(m_error_msg, _countof(m_error_msg), msg, std::forward<TArgs>(args)...);
     }
   //--- forbidden
                     CInterClient(CInterClient&)=delete;
   CInterClient&     operator=(const CInterClient&)=delete;

   //+------------------------------------------------------------------+
   //| Helper class for managing buffers in pipe                        |
   //+------------------------------------------------------------------+
   struct UberWritter
     {
      CInterClient&     client;                          // our client reference
      uint8_t           local_buf[STACK_BUF_SIZE]={};    // small static buffer to optimize pipe system calls
      uint32_t          buf_pos=0;                       // current position in local buffer
      uint32_t          buf_remain=STACK_BUF_SIZE;       // remain free size in local buffer

                        UberWritter(CInterClient& client_) : client(client_) {}
      //+------------------------------------------------------------------+
      //| Internal pipe adding                                             |
      //+------------------------------------------------------------------+
      bool InnerAdd(const void *mem, uint32_t size)
        {
         //--- if local buffer to small send it and try again
         if(size > buf_remain)
           {
            //--- fill entirely our buffer
            if(buf_remain > 0)
              {
               memcpy(&local_buf[buf_pos], mem, buf_remain);
               mem=(uint8_t*)mem + buf_remain;
               size-=buf_remain;
              }
            //--- move buffer to start
            buf_pos=0;
            buf_remain=STACK_BUF_SIZE;
            //--- send complete buffer
            DWORD written=0;
            if(WriteFile(client.m_pipe, local_buf, STACK_BUF_SIZE, &written, nullptr)==0 || written!=STACK_BUF_SIZE)
              {
               client.ProcessPipeError(GetLastError());
               return(false);
              }
            //--- ok if adding buffer too big, just send it directly from user buffer
            if(size > STACK_BUF_SIZE)
              {
               if(WriteFile(client.m_pipe, mem, size, &written, nullptr)==0 || written!=size)
                 {
                  client.ProcessPipeError(GetLastError());
                  return(false);
                 }
               //--- success
               return(true);
              }
           }
         //--- just append to end
         memcpy(&local_buf[buf_pos], mem, size);
         buf_pos+=size;
         buf_remain-=size;
         //--- success
         return(true);
        }
      //+------------------------------------------------------------------+
      //| Variadic end                                                     |
      //+------------------------------------------------------------------+
      constexpr static uint32_t CalcSize()
        {
         return(0);
        }
      //+------------------------------------------------------------------+
      //| Calculating size of variables in variadic                        |
      //+------------------------------------------------------------------+
      template <typename... TRest>
      constexpr static uint32_t CalcSize(const EnBlockSizer, TRest&&... args)
        {
         return(CalcSize(std::forward<TRest>(args)...) + sizeof(uint32_t));
        }
      //+------------------------------------------------------------------+
      //| Calculating size of variables in variadic                        |
      //+------------------------------------------------------------------+
      template <typename... TRest>
      constexpr static uint32_t CalcSize(const UberString &val, TRest&&... args)
        {
         return(CalcSize(std::forward<TRest>(args)...) + sizeof(uint32_t) + val.len * sizeof(wchar_t));
        }
      //+------------------------------------------------------------------+
      //| Calculating size of variables in variadic                        |
      //+------------------------------------------------------------------+
      template <typename T, typename... TRest>
      constexpr static uint32_t CalcSize(const UberArray<T> &val, TRest&&... args)
        {
         return(CalcSize(std::forward<TRest>(args)...) + sizeof(uint32_t) + val.count * sizeof(T));
        }
      //+------------------------------------------------------------------+
      //| Calculating size of variables in variadic                        |
      //+------------------------------------------------------------------+
      template <typename T, typename... TRest>
      constexpr static uint32_t CalcSize(const T&, TRest&&... args)
        {
         //--- check if T is integer or float
         //static_assert(std::is_integral<T>::value || std::is_enum<T>::value || std::is_floating_point<T>::value, "Only integer, floating point and enum argument is allowed");
         static_assert(std::is_trivial<T>::value, "Only trivially copyable arguments allowed");
         return(CalcSize(std::forward<TRest>(args)...) + sizeof(T));
        }
      //+------------------------------------------------------------------+
      //| Calculating size of variables in variadic                        |
      //+------------------------------------------------------------------+
      template <typename T, size_t t_count, typename... TRest>
      constexpr static uint32_t CalcSize(const T(&val)[t_count], TRest&&... args)
        {
         //--- check if T is integer or float
         static_assert(std::is_integral<T>::value || std::is_enum<T>::value || std::is_floating_point<T>::value, "Only integer, floating point and enum argument is allowed");
         return(CalcSize(std::forward<TRest>(args)...) + t_count * sizeof(T));
        }
      //+------------------------------------------------------------------+
      //| Variadic end                                                     |
      //+------------------------------------------------------------------+
      bool AddVariable()
        {
         //--- flush all remain buffer
         DWORD written=0;
         if(WriteFile(client.m_pipe, local_buf, buf_pos, &written, nullptr)==0 || written!=buf_pos)
           {
            client.ProcessPipeError(GetLastError());
            return(false);
           }
         //---
         return(true);
        }
      //+------------------------------------------------------------------+
      //| Variadic adding                                                  |
      //+------------------------------------------------------------------+
      template <typename... TRest>
      bool AddVariable(const EnBlockSizer, TRest&&... args)
        {
         //--- calculate size again (theoretically we can use BlockSizer as struct, not enum)
         uint32_t size=CalcSize(std::forward<TRest>(args)...);
         if(!InnerAdd(&size, sizeof(size)))
            return(false);
         //--- forward
         return(AddVariable(std::forward<TRest>(args)...));
        }
      //+------------------------------------------------------------------+
      //| Variadic adding                                                  |
      //+------------------------------------------------------------------+
      template <typename... TRest>
      bool AddVariable(const UberString &val, TRest&&... args)
        {
         if(!InnerAdd(&val.len, sizeof(uint32_t)))
            return(false);
         if(!InnerAdd(val.text, val.len * sizeof(wchar_t)))
            return(false);
         //--- forward
         return(AddVariable(std::forward<TRest>(args)...));
        }
      //+------------------------------------------------------------------+
      //| Variadic adding                                                  |
      //+------------------------------------------------------------------+
      template <typename T, typename... TRest>
      bool AddVariable(const UberArray<T> &val, TRest&&... args)
        {
         if(!InnerAdd(&val.count, sizeof(uint32_t)))
            return(false);
         if(!InnerAdd(val.data, val.count * sizeof(T)))
            return(false);
         //--- forward
         return(AddVariable(std::forward<TRest>(args)...));
        }
      //+------------------------------------------------------------------+
      //| Variadic adding                                                  |
      //+------------------------------------------------------------------+
      template <typename T, typename... TRest>
      bool AddVariable(const T &val, TRest&&... args)
        {
         static_assert(std::is_trivial<T>::value, "Only trivially copyable arguments allowed");
         //---
         if(!InnerAdd(&val, sizeof(T)))
            return(false);
         //--- forward
         return(AddVariable(std::forward<TRest>(args)...));
        }
      //+------------------------------------------------------------------+
      //| Variadic adding                                                  |
      //+------------------------------------------------------------------+
      template <typename T, size_t t_count, typename... TRest>
      bool AddVariable(const T(&val)[t_count], TRest&&... args)
        {
         uint32_t tmp=t_count;
         if(!InnerAdd(&tmp, sizeof(tmp)))
            return(false);
         if(!InnerAdd(&val, t_count * sizeof(T)))
            return(false);
         //--- forward
         return(AddVariable(std::forward<TRest>(args)...));
        }
      //+------------------------------------------------------------------+
      //| Sending one message                                              |
      //+------------------------------------------------------------------+
      template <typename... TArgs>
      bool Send(TArgs&&... args)
        {
         //--- we need to calculate message size
         uint32_t msg_size=CalcSize(std::forward<TArgs>(args)...);
         //--- add message size
         memcpy(local_buf, &msg_size, sizeof(uint32_t));
         buf_pos+=sizeof(uint32_t);
         buf_remain-=sizeof(uint32_t);
         //--- forwarding variadic recursion
         return(AddVariable(std::forward<TArgs>(args)...));
        }
     };
   //+------------------------------------------------------------------+
   //| Helper class for managing buffers in pipe                        |
   //+------------------------------------------------------------------+
   struct UberReader
     {
      CInterClient&     client;                          // our client reference
      uint8_t           local_buf[STACK_BUF_SIZE]={};    // small static buffer to optimize pipe system calls
      uint32_t          msg_remain=0;                    // total message size (remaining)
      uint32_t          buf_pos=0;                       // current position in local buffer
      uint32_t          buf_remain=0;                    // used size in local buffer

                        UberReader(CInterClient& client_) : client(client_) {}
      //+------------------------------------------------------------------+
      //| Getting one variable                                             |
      //+------------------------------------------------------------------+
      bool InnerRead(void *mem, uint32_t size)
        {
         if(size<=buf_remain)
           {
            memcpy(mem, &local_buf[buf_pos], size);
            //--- move
            buf_pos+=size;
            buf_remain-=size;
            //---
            return(true);
           }
         //--- copy remaing
         if(buf_remain > 0)
           {
            memcpy(mem, &local_buf[buf_pos], buf_remain);
            //--- correct copy buffer
            mem=(uint8_t*)mem + buf_remain;
            size-=buf_remain;
            buf_remain=0;
           }
         buf_pos=0;
         //--- check total message size
         if(size > msg_remain)
            return(false);
         //--- if we receive too big size, direct receive to user buffer
         if(size>=STACK_BUF_SIZE/2 || size==msg_remain)
           {
            DWORD readbytes;
            if(ReadFile(client.m_pipe, mem, size, &readbytes, nullptr)==0 || readbytes!=size)
              {
               client.ProcessPipeError(GetLastError());
               return(false);
              }
            //--- correct sizes
            msg_remain-=size;
           }
         else
           {
            //--- use intermediate buffer
            uint32_t to_read=(std::min)(uint32_t(STACK_BUF_SIZE), msg_remain);
            DWORD readbytes;
            if(ReadFile(client.m_pipe, local_buf, to_read, &readbytes, nullptr)==0 || readbytes!=to_read)
              {
               client.ProcessPipeError(GetLastError());
               return(false);
              }
            //--- copy our part
            memcpy(mem, &local_buf[buf_pos], size);
            //--- move
            buf_pos+=size;
            buf_remain=to_read - size;
            msg_remain-=to_read;
           }
         //---
         return(true);
        }
      //+------------------------------------------------------------------+
      //| Getting one variable (ender)                                     |
      //+------------------------------------------------------------------+
      bool GetVariable(void)
        {
         return(true);
        }
      //+------------------------------------------------------------------+
      //| Getting one variable (template value)                            |
      //+------------------------------------------------------------------+
      template <typename T, typename... TRest>
      bool GetVariable(T *val, TRest&&... args)
        {
         //--- check if T is integer or float
         //static_assert(std::is_integral<T>::value || std::is_enum<T>::value || std::is_floating_point<T>::value, "Only integer, floating point, enum argument allowed");
         static_assert(std::is_trivial<T>::value, "Only trivially copyable arguments allowed");
         //--- if we don't have in buffer - proceed to full version
         if(sizeof(T)<=buf_remain)
           {
            //--- get variable
            if(val)
               memcpy(val, &local_buf[buf_pos], sizeof(T));
            //--- move
            buf_pos+=sizeof(T);
            buf_remain-=sizeof(T);
           }
         else
            if(!InnerRead(val, sizeof(T)))
               return(false);
         //--- forward variadic
         return(GetVariable(std::forward<TRest>(args)...));
        }
      //+------------------------------------------------------------------+
      //| Getting one variable (template array)                            |
      //+------------------------------------------------------------------+
      template <typename T, size_t t_count, typename... TRest>
      bool GetVariable(T(*val)[t_count], TRest&&... args)
        {
         //--- check if T is integer or float
         static_assert(std::is_integral<T>::value || std::is_enum<T>::value || std::is_floating_point<T>::value, "Only integer, floating point, enum argument allowed");
         if(!InnerRead(val, sizeof(T) * t_count))
            return(false);
         //--- forward variadic
         return(GetVariable(std::forward<TRest>(args)...));
        }
      //+------------------------------------------------------------------+
      //| Getting one variable (mapper)                                    |
      //+------------------------------------------------------------------+
      template <typename T, typename TAllocator, typename... TRest>
      bool GetVariable(const UberMapper<T, TAllocator> &val, TRest&&... args)
        {
         uint32_t count=0;
         //--- take a length
         if(sizeof(uint32_t)<=buf_remain)
           {
            memcpy(&count, &local_buf[buf_pos], sizeof(uint32_t));
            buf_pos+=sizeof(uint32_t);
            buf_remain-=sizeof(uint32_t);
           }
         else
            if(!InnerRead(&count, sizeof(count)))
               return(false);
         //--- allocate
         void *mem=val.allocator(sizeof(T) * count);
         if(mem==nullptr)
            return(false);
         //--- copy data
         if(!InnerRead(mem, count * sizeof(T)))
            return(false);
         //--- save count
         if(val.count)
            *val.count=count;
         if(val.data)
            *val.data=(T*)mem;
         //--- forward variadic
         return(GetVariable(std::forward<TRest>(args)...));
        }
      //+------------------------------------------------------------------+
      //| Getting full message                                             |
      //+------------------------------------------------------------------+
      template <typename... TArgs>
      bool Recv(TArgs&&... args)
        {
         //--- always read size of message
         DWORD readbytes=0;
         if(ReadFile(client.m_pipe, &msg_remain, sizeof(msg_remain), &readbytes, nullptr)==0 || readbytes!=sizeof(msg_remain))
           {
            client.ProcessPipeError(GetLastError());
            return(false);
           }
         //--- forward variadic recursion and check for completeness
         return(GetVariable(std::forward<TArgs>(args)...) && buf_remain==0 && msg_remain==0);
        }
     };

public:
                     CInterClient(void);
                    ~CInterClient(void);

   bool              Initialize(const wchar_t *name);
   void              Shutdown(void);

   bool              Send(const void* data, UINT32 datasize);
   void*             Recv(UINT32 *datasize);
   void              Free(void *mem);

   bool              IsValid() { return(m_pipe!=INVALID_HANDLE_VALUE); }

   //+------------------------------------------------------------------+
   //| Variadic adding                                                  |
   //+------------------------------------------------------------------+
   template <typename... TArgs>
   bool SendVars(TArgs&&... args)
     {
      return(UberWritter(*this).Send(std::forward<TArgs>(args)...));
     }
   //+------------------------------------------------------------------+
   //| Variadic reading                                                 |
   //+------------------------------------------------------------------+
   template <typename... TArgs>
   bool RecvVars(TArgs&&... args)
     {
      return(UberReader(*this).Recv(std::forward<TArgs>(args)...));
     }

#if(_WINVER>=0x0600)
   DWORD             GetServerProcessId();
#endif
   const wchar_t*    ErrorString() const { return(m_error_msg); }

private:
   bool              InnerPipeCreate(const wchar_t *pipe_name);
   bool              InnerInitializeAppData(bool *try_again);
   bool              InnerInitializeProgramFiles(bool *try_again);
   bool              InnerInitialize(const wchar_t *path, bool *try_again);
   void              ProcessPipeError(DWORD err_code);
  };
//+------------------------------------------------------------------+
