/*
   fnmock.cpp -- implementation of the 'fnmock', a simple function mocker for x86 & x86_64
   Copyright (c) 2013, Fengping Bao <jamol@live.com>
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "fnmock.h"
#if defined(WIN32)
#include <windows.h>
#elif defined(LINUX)
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#elif defined(MACOS)
#error "unsupport OS"
#else
#error "unsupport OS"
#endif

#include <map>

#define JMP_OPCODE_SIZE   5
#define JMP64_OPCODE_SIZE 12

typedef struct tagMockData{
    void* srcFunc;
    void* dstFunc;
    unsigned char code[JMP64_OPCODE_SIZE];
}MockData;

typedef std::map<void*, MockData>   MockDataMap;
static MockDataMap s_mockDataMap;

MockData getMockData(void* func)
{
    MockData mockData = {0};
    MockDataMap::iterator it = s_mockDataMap.find(func);
    if(it != s_mockDataMap.end())
    {
        return it->second;
    }
    return mockData;
}

bool osBit64() 
{
    return sizeof(void*)==8;
}

int getPageSize()
{
    static int s_page_size = 0;
    if(0 == s_page_size)
    {    
#if defined(LINUX)
        s_page_size = sysconf(_SC_PAGE_SIZE);
        if(-1 == s_page_size)
            s_page_size = osBit64()?8192:4096;
#else
        s_page_size = osBit64()?8192:4096;
#endif
    }
    return s_page_size;
}

int fnMock(void* srcFunc, void* dstFunc)
{
    MockData mockData = getMockData(srcFunc);
    if(mockData.srcFunc != NULL)
    {
        if(mockData.dstFunc == dstFunc)
        {
            return 0;
        }
        else
        {
            fnUnmock(srcFunc);
        }
    }
    int opSize = JMP_OPCODE_SIZE;
    if(osBit64())
        opSize = JMP64_OPCODE_SIZE;
#if defined(WIN32)
    DWORD oldProtect = 0;
    VirtualProtect(srcFunc, opSize, PAGE_EXECUTE_READWRITE, &oldProtect);
#elif defined(LINUX)
    int page_size = getPageSize();
    char* pStart = (char *)((long)srcFunc / page_size * page_size);
    int nPage = ((long)srcFunc + opSize - (long)pStart + page_size - 1) / page_size * page_size;

    int res = mprotect(pStart, nPage, PROT_WRITE | PROT_READ | PROT_EXEC);
    if(0 != res)
    {
        return -1;
    }
#elif defined(MACOS)
#error "unsupport OS"
#else
#error "unsupport OS"
#endif
    memcpy(mockData.code, srcFunc, opSize);
    unsigned char* p = (unsigned char*)srcFunc;
    if(!osBit64())
    {
        void* offset = (void*)((unsigned char*)dstFunc - (unsigned char*)srcFunc - opSize);
        p[0] = 0xe9;
        *(void**)(p + 1) = offset;
    }
    else
    {
        // mov rax, offset
        int idx = 0;
        p[idx++] = 0x48;
        p[idx++] = 0xb8;
        *((void**)(p + idx)) = (void*)dstFunc;
        idx += sizeof(void*);
        p[idx++] = 0xff;
        p[idx++] = 0xe0;
    }
    mockData.srcFunc = srcFunc;
    mockData.dstFunc = dstFunc;
    s_mockDataMap.insert(std::make_pair(srcFunc, mockData));
    return 0;
}

void fnUnmock(void* srcFunc)
{
    MockData mockData = getMockData(srcFunc);
    if(mockData.srcFunc == NULL)
        return ;
    int opSize = JMP_OPCODE_SIZE;
    if(osBit64())
        opSize = JMP64_OPCODE_SIZE;
    memcpy(srcFunc, mockData.code, opSize);
    s_mockDataMap.erase(srcFunc);
}
