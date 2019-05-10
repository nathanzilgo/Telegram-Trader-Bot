//+------------------------------------------------------------------+
//|                                                     MetaTrader 5 |
//|                   Copyright 2000-2019, MetaQuotes Software Corp. |
//|                                        http://www.metaquotes.net |
//+------------------------------------------------------------------+
#include "stdafx.h"
#include "InterClient.h"
//+------------------------------------------------------------------+
//| Helper structure                                                 |
//+------------------------------------------------------------------+
struct TerminalFilePath
  {
   wchar_t           path[MAX_PATH + 1]={};
   FILETIME          modify_date={};

   bool operator < (const TerminalFilePath &other) const
     {
      return(modify_date.dwHighDateTime > other.modify_date.dwHighDateTime || (modify_date.dwHighDateTime==other.modify_date.dwHighDateTime && modify_date.dwLowDateTime > other.modify_date.dwLowDateTime));
     }
  };
//+------------------------------------------------------------------+
//| Constructor                                                      |
//+------------------------------------------------------------------+
CInterClient::CInterClient(void)
  {
  }
//+------------------------------------------------------------------+
//| Destructor                                                       |
//+------------------------------------------------------------------+
CInterClient::~CInterClient(void)
  {
   Shutdown();
  }
//+------------------------------------------------------------------+
//| Shutdown clearance                                               |
//+------------------------------------------------------------------+
void CInterClient::Shutdown(void)
  {
   if(m_pipe!=INVALID_HANDLE_VALUE)
     {
      CloseHandle(m_pipe);
      m_pipe=INVALID_HANDLE_VALUE;
     }
//---
   ZeroMemory(m_unique_hash, sizeof(m_unique_hash));
   ZeroMemory(m_error_msg, sizeof(m_error_msg));
  }
//+------------------------------------------------------------------+
//| Calculate SHA256 hash                                            |
//+------------------------------------------------------------------+
static bool SHA256Hash(const void *data, UINT32 data_size, BYTE(*out)[32])
  {
   if(out==nullptr)
      return(false);

   HCRYPTPROV hprov={};
//--- create provider
   if(CryptAcquireContext(&hprov, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)==0)
      return(false);

   HCRYPTHASH hhash={};
//--- create hash algorithm
   if(CryptCreateHash(hprov, CALG_SHA_256, 0, 0, &hhash)==0)
     {
      CryptReleaseContext(hprov, 0);
      return(false);
     }

   bool bres=false;
//--- hashing
   if(CryptHashData(hhash, (const BYTE*)data, data_size, 0)!=0)
     {
      DWORD size=sizeof(*out);
      if(CryptGetHashParam(hhash, HP_HASHVAL, *out, &size, 0)!=0)
         bres=true;
     }
//--- clearance
   CryptDestroyHash(hhash);
   CryptReleaseContext(hprov, 0);

   return(bres);
  }
//+------------------------------------------------------------------+
//| Building hash string                                             |
//+------------------------------------------------------------------+
static bool HashToName(const wchar_t *prefix, const BYTE(&hash)[32], wchar_t *out, UINT32 out_len)
  {
   if(out==nullptr || out_len<=0)
      return(false);
//---
   UINT32 prefix_len=(UINT32)wcslen(prefix);
   if(out_len < prefix_len + 2 * _countof(hash) + 1)
     {
      *out=L'\0';
      return(false);
     }
//---
   wmemcpy(out, prefix, prefix_len);
   out+=prefix_len;
//---
   for(UINT32 idx=0; idx < _countof(hash); idx++)
     {
      BYTE val=hash[idx] >> 4;
      if(val>=10)
         *(out++)=(val - 10) + L'A';
      else
         *(out++)=val + L'0';
      //---
      val=hash[idx] & 0xF;
      if(val>=10)
         *(out++)=(val - 10) + L'A';
      else
         *(out++)=val + L'0';
     }
//---
   *(out++)=L'\0';
   return(true);
  }
//+------------------------------------------------------------------+
//| Unificate path by system                                         |
//+------------------------------------------------------------------+
template <size_t t_len> static bool UnificatePath(const wchar_t *path, wchar_t(*res_path)[t_len])
  {
   if(path==nullptr || res_path==nullptr)
      return(false);
//--- first try open by handle
   bool bres=false;
   HANDLE hfile=CreateFile(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING | FILE_SHARE_WRITE, 0, nullptr);
   if(hfile!=INVALID_HANDLE_VALUE)
     {
      DWORD len=GetFinalPathNameByHandle(hfile, *res_path, _countof(*res_path) - 1, FILE_NAME_NORMALIZED);
      if(len > 0 && len < _countof(*res_path))
        {
         (*res_path)[len]=L'\0';
         bres=true;
        }
      CloseHandle(hfile);
     }
//--- just copy on error
   if(!bres)
     {
      size_t len=wcslen(path);
      if(len>=_countof(*res_path))
         len=_countof(*res_path) - 1;
      wmemmove(*res_path, path, len);
      (*res_path)[len]=L'\0';
     }

   return(true);
  }
//+------------------------------------------------------------------+
//| Internal initialization (special for several tries)              |
//+------------------------------------------------------------------+
bool CInterClient::InnerInitialize(const wchar_t *path, bool *try_again)
  {
//--- checks
   if(try_again)
      *try_again=false;
   if(path==nullptr)
      return(false);

   wchar_t local_text[MAX_PATH + 1]={};
//--- pipe unique name generating
   if(wcslen(path) > _countof(local_text) - 1)
     {
      SetErrorMsg(L"Server name too long");
      return(false);
     }
//--- generate lowercase name
   UnificatePath(path, &local_text);
   UINT32 path_len=(UINT32)wcslen(local_text);
   CharLowerBuff(local_text, path_len);
//--- SHA256 hash
   if(!SHA256Hash(local_text, path_len * sizeof(wchar_t), &m_unique_hash))
     {
      SetErrorMsg(L"Hash calculation failed");
      return(false);
     }
//--- generate unique name
   wchar_t pipe_name[MAX_PATH]={};
   HashToName(L"\\\\.\\pipe\\MT5.Terminal.", m_unique_hash, pipe_name, _countof(pipe_name));
//--- trying connect first time
   if(!InnerPipeCreate(pipe_name) || m_pipe==INVALID_HANDLE_VALUE)
     {
      PROCESS_INFORMATION pi={};
      STARTUPINFO si={};
      si.cb=sizeof(si);
      si.hStdInput=INVALID_HANDLE_VALUE;
      si.hStdOutput=INVALID_HANDLE_VALUE;
      si.hStdError=INVALID_HANDLE_VALUE;
      si.dwFlags=STARTF_USESTDHANDLES;
      //--- trying to launch MetaTrader5 Terminal
      if(CreateProcess(path, nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)==0)
        {
         if(try_again)
            *try_again=true;
         SetErrorMsg(L"Process create failed '%s'", path);
         return(false);
        }
      //--- close handles
      if(pi.hThread)
         CloseHandle(pi.hThread);
      if(pi.hProcess)
         CloseHandle(pi.hProcess);
      //--- check pipe presents every 1sec 
      for(UINT32 tick=0; tick < MAX_TERMINAL_LAUNCH_TIME; tick+=1000)
        {
         if(InnerPipeCreate(pipe_name) && m_pipe!=INVALID_HANDLE_VALUE)
            break;
         //---
         Sleep(1000);
        }
      //--- check if connected
      if(m_pipe==INVALID_HANDLE_VALUE)
        {
         SetErrorMsg(L"Pipe server didn't answer in %u msec", UINT32(MAX_TERMINAL_LAUNCH_TIME));
         return(false);
        }
     }
//--- success
   return(true);
  }
//+------------------------------------------------------------------+
//| Initialize/start terminals from %appdata%                        |
//+------------------------------------------------------------------+
bool CInterClient::InnerInitializeAppData(bool *ptry_again)
  {
//--- get appdata
   bool bres=false, try_again=true;
   PWSTR folder=nullptr;
   if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &folder)) && folder)
     {
      TerminalFilePath items[30]={};
      UINT32 items_count=0;
      wchar_t local_path[MAX_PATH + 1]={};
      wchar_t search_path[MAX_PATH + 1]={};
      wcscpy_s(local_path, folder);
      wcscat_s(local_path, L"\\MetaQuotes\\Terminal\\");
      //--- prepare search path
      wcscpy_s(search_path, local_path);
      wcscat_s(search_path, L"*");
      WIN32_FIND_DATA wfd={};
      HANDLE hfind=FindFirstFile(search_path, &wfd);
      if(hfind!=INVALID_HANDLE_VALUE)
        {
         do
           {
            //--- process folders only
            if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)
               continue;
            //--- skip any points, we don't use such folders anyway
            if(wfd.cFileName[0]==L'.')
               continue;

            wchar_t buf[MAX_PATH + 2];
            wchar_t temp_path[MAX_PATH + 1];
            wcscpy_s(temp_path, local_path);
            wcscat_s(temp_path, wfd.cFileName);
            wcscat_s(temp_path, L"\\MQL5");
            //--- check folder has MQL5 folder
            DWORD attr=GetFileAttributes(temp_path);
            if(attr==INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)==0)
               continue;
            //--- open "origin.txt"
            wcscpy_s(temp_path, local_path);
            wcscat_s(temp_path, wfd.cFileName);
            wcscat_s(temp_path, L"\\origin.txt");
            HANDLE hfile=CreateFile(temp_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
            if(hfile==INVALID_HANDLE_VALUE)
               continue;
            //--- read data
            DWORD r;
            if(ReadFile(hfile, buf, sizeof(buf) - sizeof(wchar_t), &r, nullptr)!=0)
              {
               wchar_t *it=buf;
               UINT32 len=r/sizeof(wchar_t);
               if(len > 1)
                 {
                  //--- skip BOM
                  if(*it==L'\xFEFF')
                    {
                     it++;
                     len--;
                    }
                  if(len<=MAX_PATH)
                    {
                     it[len]=L'\0';
                     //--- add to array
                     wcscpy_s(items[items_count].path, it);
                     items[items_count].modify_date=wfd.ftLastWriteTime;
                     //--- try to receive actual last launch time
                     wcscpy_s(temp_path, local_path);
                     wcscat_s(temp_path, wfd.cFileName);
                     wcscat_s(temp_path, L"\\config\\accounts.dat");
                     WIN32_FILE_ATTRIBUTE_DATA wfad={};
                     if(GetFileAttributesEx(temp_path, GetFileExInfoStandard, &wfad)!=0)
                        items[items_count].modify_date=wfad.ftLastWriteTime;
                     //--- check boundary
                     if(++items_count>=_countof(items))
                        break;
                    }
                 }
              }
            //--- close handle
            CloseHandle(hfile);
           } while(FindNextFile(hfind, &wfd));
         //---
         FindClose(hfind);
        }
      //--- free memory
      CoTaskMemFree(folder);
      //--- sort by time stored pathes
      std::sort(items, items + items_count);
      //--- process stored pathes
      for(UINT32 idx=0; idx < items_count; idx++)
        {
         //--- try to connect (64-bit)
         wcscpy_s(local_path, items[idx].path);
         wcscat_s(local_path, L"\\terminal64.exe");
         bres=InnerInitialize(local_path, &try_again);
         //--- try 32-bit too
         if(!bres && try_again)
           {
            wcscpy_s(local_path, items[idx].path);
            wcscat_s(local_path, L"\\terminal.exe");
            bres=InnerInitialize(local_path, &try_again);
           }
         //--- stop on error or success
         if(bres || !try_again)
            break;
        }
     }
//--- save try_again
   if(ptry_again)
      *ptry_again=try_again;
//--- result
   return(bres);
  }
//+------------------------------------------------------------------+
//| Initialize/start terminals from Program Files                    |
//+------------------------------------------------------------------+
bool CInterClient::InnerInitializeProgramFiles(bool *ptry_again)
  {
   wchar_t local_path[MAX_PATH + 1]={};
   bool bres=false, try_again=true;
   PWSTR folder=nullptr;
   if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, 0, nullptr, &folder)) && folder)
     {
      wcscpy_s(local_path, folder);
      wcscat_s(local_path, L"\\MetaTrader 5\\terminal.exe");
      bres=InnerInitialize(local_path, &try_again);
      //--- free memory
      CoTaskMemFree(folder);
      folder=nullptr;
     }
//--- try 32bit
   if(!bres && try_again)
     {
      if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFilesX64, 0, nullptr, &folder)) && folder)
        {
         wcscpy_s(local_path, folder);
         wcscat_s(local_path, L"\\MetaTrader 5\\terminal64.exe");
         bres=InnerInitialize(local_path, &try_again);
         //--- free memory
         CoTaskMemFree(folder);
         folder=nullptr;
        }
      //--- try constants
      if(!bres && try_again)
        {
         wcscpy_s(local_path, L"C:\\Program Files\\MetaTrader 5\\terminal64.exe");
         bres=InnerInitialize(local_path, &try_again);
        }
     }
//--- save try_again
   if(ptry_again)
      *ptry_again=try_again;
//--- result
   return(bres);
  }
//+------------------------------------------------------------------+
//| Initialization                                                   |
//+------------------------------------------------------------------+
bool CInterClient::Initialize(const wchar_t *path)
  {
   if(m_pipe!=INVALID_HANDLE_VALUE)
     {
      SetErrorMsg(L"CInterClient already initialized");
      return(false);
     }
//--- checks
   if(path==nullptr || path[0]==L'\0')
     {
      bool try_again=true;
      if(InnerInitializeAppData(&try_again))
         return(true);
      else
         if(try_again)
            return(InnerInitializeProgramFiles(nullptr));
      //--- failed
      return(false);
     }
   else
      return(InnerInitialize(path, nullptr));
  }
//+------------------------------------------------------------------+
//| Internal pipe creation                                           |
//+------------------------------------------------------------------+
bool CInterClient::InnerPipeCreate(const wchar_t *pipe_name)
  {
   while(1)
     {
      //--- opening pipe
      if((m_pipe=CreateFile(pipe_name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr))!=INVALID_HANDLE_VALUE)
         break;
      //--- save last error
      DWORD dwerr=GetLastError();
      //--- special case, we have pipe, but it not free yet
      if(dwerr!=ERROR_PIPE_BUSY || WaitNamedPipe(pipe_name, NMPWAIT_WAIT_FOREVER)==0)
        {
         SetErrorMsg(L"CreateFile failed, [%u]", dwerr);
         return(false);
        }
     }
//--- success
   return(true);
  }
//+------------------------------------------------------------------+
//| Send one entire packet                                           |
//+------------------------------------------------------------------+
bool CInterClient::Send(const void* data, UINT32 datasize)
  {
   if(m_pipe==INVALID_HANDLE_VALUE)
     {
      SetErrorMsg(L"Trying using uninitialized pipe");
      return(false);
     }
//---
   BYTE local_buf[4096];
   if(datasize < sizeof(local_buf) - sizeof(UINT32))
     {
      //--- replace buffer to local with filled size, remove one system call
      memcpy(local_buf, &datasize, sizeof(UINT32));
      memcpy(&local_buf[sizeof(UINT32)], data, datasize);
      data=local_buf;
      datasize+=sizeof(UINT32);
     }
   else
     {
      //--- wtrite size of message
      DWORD written=0;
      if(WriteFile(m_pipe, &datasize, sizeof(datasize), &written, nullptr)==0 || written!=sizeof(datasize))
        {
         DWORD dwerr=GetLastError();
         ProcessPipeError(dwerr);
         SetErrorMsg(L"WriteFile failed(1), [%u]", dwerr);
         return(false);
        }
     }
//--- write message itself
   DWORD written=0;
   if(WriteFile(m_pipe, data, datasize, &written, nullptr)==0 || written!=datasize)
     {
      DWORD dwerr=GetLastError();
      ProcessPipeError(dwerr);
      SetErrorMsg(L"WriteFile failed(2), [%u]", dwerr);
      return(false);
     }
//---
   return(true);
  }
//+------------------------------------------------------------------+
//| Receive one entire message                                       |
//| memory must be freed by FreeMessage                              |
//+------------------------------------------------------------------+
void* CInterClient::Recv(UINT32 *datasize)
  {
   if(m_pipe==INVALID_HANDLE_VALUE)
     {
      SetErrorMsg(L"Trying using uninitialized pipe");
      return(nullptr);
     }
//--- receiving size
   UINT32 msgsize=0;
   DWORD readbytes=0;
   if(ReadFile(m_pipe, &msgsize, sizeof(msgsize), &readbytes, nullptr)==0 || readbytes!=sizeof(msgsize))
     {
      DWORD dwerr=GetLastError();
      ProcessPipeError(dwerr);
      SetErrorMsg(L"ReadFile failed(1), [%u]", dwerr);
      return(nullptr);
     }
//--- buffer allocating
   void *mem=new(std::nothrow) char[msgsize];
   if(mem==nullptr)
     {
      SetErrorMsg(L"No memory");
      return(nullptr);
     }
//--- reading data itself
   if(ReadFile(m_pipe, mem, msgsize, &readbytes, nullptr)==0 || readbytes!=msgsize)
     {
      DWORD dwerr=GetLastError();
      ProcessPipeError(dwerr);
      SetErrorMsg(L"ReadFile failed(2), [%u]", dwerr);
      return(nullptr);
     }
//--- setting size
   if(datasize)
      *datasize=msgsize;

   return(mem);
  }
//+------------------------------------------------------------------+
//| Freeing allocated memory                                         |
//+------------------------------------------------------------------+
void CInterClient::Free(void *mem)
  {
   if(mem)
      delete[] (char*)mem;
  }
//+------------------------------------------------------------------+
//| Processing pipe errors, some error lead to pipe close            |
//+------------------------------------------------------------------+
void CInterClient::ProcessPipeError(DWORD err_code)
  {
   if(err_code==ERROR_BROKEN_PIPE)
     {
      //--- we must close pipe from our side, next calls must fail with "No IPC connection"
      CloseHandle(m_pipe);
      m_pipe=INVALID_HANDLE_VALUE;
     }
  }
#if(_WINVER>=0x0600)
//+------------------------------------------------------------------+
//| Getting process ID from pipe                                     |
//+------------------------------------------------------------------+
DWORD CInterClient::GetServerProcessId()
  {
   ULONG process_id=0;
   if(GetNamedPipeServerProcessId(m_pipe, &process_id)==0)
     {
      SetErrorMsg(L"GetNamedPipeClientProcessId failed");
      return(0);
     }

   return(process_id);
  }
#endif
//+------------------------------------------------------------------+
