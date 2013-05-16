/*
   ufmock.h -- interface of the 'ufmock', a universal function mocker for x86 & x86_64
   Copyright (c) 2013, Fengping Bao <jamol@live.com>, Linfeng Wen <linfengwen@gmail.com>
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

#ifndef  __UNIVERSAL_FUNCTION_MOCKER_H__
#define  __UNIVERSAL_FUNCTION_MOCKER_H__

#include "distorm.h"
#include "fnmock.h"
#include "traits.h"

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

template <typename ReturnType>
class UFMocker
{
private:
    void*  m_orig_func; // original function pointer

    int		m_alloc_size;
    void*	m_mocked_func; // mocked function pointer
    void*	m_return_data; // return data offset inside m_mocked_func

    typedef enum eReturnType 
    {
        E_TYPE_INTEGER,
        E_TYPE_FLOAT,
        E_TYPE_STRUCT,
        E_TYPE_REFERENCE,
    }E_RETURN_TYPE;

    E_RETURN_TYPE m_return_type;

public:
    UFMocker()
    {
        m_orig_func = NULL;
        m_alloc_size = 0;
        m_mocked_func = NULL;
        m_return_data = NULL;

        if(ufmock::is_class<ReturnType>::value)
            m_return_type = E_TYPE_STRUCT;
        else if(ufmock::is_float<ReturnType>::value)
            m_return_type = E_TYPE_FLOAT;
        else if(ufmock::is_reference<ReturnType>::value)
            m_return_type = E_TYPE_REFERENCE;
        else 
            m_return_type = E_TYPE_INTEGER;
    }

    ~UFMocker()
    {
        unmock();
    }

private:
    void cleanup()
    {
        if(NULL != m_mocked_func)
        {
#if defined(WIN32)
            free(m_mocked_func);
#elif defined(LINUX)
            munmap(m_mocked_func, m_alloc_size);
#elif defined(MACOS)
#error "unsupport OS"
#else
#error "unsupport OS"
#endif
            m_mocked_func = NULL;
        }
    }

    int getRetCode32(void* func, unsigned char *buf)
    {
#define MAX_INST_SIZE	20
#define CODE_BLOCK_SIZE	32
#define MAX_SCAN_LENGTH	20*1024*1024 // 20 MB
        _DecodeType type = osBit64() ? Decode64Bits : Decode32Bits;
        _DecodedInst inst[MAX_INST_SIZE];
        unsigned int count = 0;
        unsigned int scaned_length = 0;
        unsigned char* p = (unsigned char*)func;
        while(scaned_length < MAX_SCAN_LENGTH)
        {
#ifdef WIN32
            DWORD old_protect = 0;
            VirtualProtect(p, CODE_BLOCK_SIZE, PAGE_EXECUTE_READ, &old_protect);
#endif
            distorm_decode64(0, p, CODE_BLOCK_SIZE, type, inst, MAX_INST_SIZE, &count);
#ifdef WIN32            
            VirtualProtect(p, CODE_BLOCK_SIZE, old_protect, &old_protect);
#endif
            int cur_len = 0; // fix distorm_decode64 decode incomplete issue
            for(unsigned int index = 0; index < count && cur_len < 20; index++)
            {
                scaned_length += inst[index].size;
                cur_len += inst[index].size;
                if(*p == 0xe9)
                {
                    p += *(unsigned int*)(p + 1) + 5;
                    break;
                }
                else if(*p == 0xea)
                {
                    p = (unsigned char*)(*(unsigned int*)(p + 1));
                }
                else if(*p == 0xeb)
                {
                    p += *(unsigned char*)(p + 1) + 2;
                    break;
                }
                else if(*p == 0xc2 || *p == 0xca)
                {
                    memcpy(buf, p, 3);
                    return 3;
                }
                else if(*p == 0xc3 || *p == 0xcb)
                {
                    buf[0] = *p;
                    return 1;
                }
                p += inst[index].size;
            }
        }
        return 0;
    }

    bool genFunction32()
    {
        int idx = 0;
        unsigned char* p = (unsigned char*)m_mocked_func;

        if(E_TYPE_INTEGER == m_return_type || E_TYPE_REFERENCE == m_return_type)
        {
            // mov eax, addr
            p[idx++] = 0xb8;
            *((void**)(p + idx)) = m_return_data;
            idx += sizeof(void*);

            // mov eax, dword ptr [eax]
            p[idx++] = 0x8b;
            p[idx++] = 0x00;

            if(sizeof(long long) == sizeof(ReturnType) && E_TYPE_REFERENCE != m_return_type)
            {// long long 
                // mov edx, addr
                p[idx++] = 0xba;
                *((void**)(p + idx)) = (unsigned char*)m_return_data + 4;
                idx += sizeof(void*);

                // mov edx, dword ptr [edx]
                p[idx++] = 0x8b;
                p[idx++] = 0x12;
            }
        }
        else if(E_TYPE_FLOAT == m_return_type)
        {
            // mov eax, addr
            p[idx++] = 0xb8;
            *((void**)(p + idx)) = m_return_data;
            idx += sizeof(void*);

            if(sizeof(float) == sizeof(ReturnType))
            {// float
                // fld qword ptr [eax]
                p[idx++] = 0xd9;
                p[idx++] = 0x00;
            }
            else if(sizeof(double) == sizeof(ReturnType))
            {// double
                // fld qword ptr [eax]
                p[idx++] = 0xdd;
                p[idx++] = 0x00;
            }
            else if(sizeof(long double) == sizeof(ReturnType))
            {// long double, linux
                // fldt qword ptr [eax]
                p[idx++] = 0xdb;
                p[idx++] = 0x28;
            }
        }
        else if(E_TYPE_STRUCT == m_return_type)
        {
            p[idx++] = 0x55; // push ebp
            p[idx++] = 0x8b;
            p[idx++] = 0xec;

            p[idx++] = 0x56; // push esi
            p[idx++] = 0x57; // push edi
            p[idx++] = 0x51; // push ecx

            // mov ecx, length
            p[idx++] = 0xb9;
            *((unsigned int*)(p + idx)) = sizeof(ReturnType);
            idx += 4;
            // mov esi, addr
            p[idx++] = 0xbe;
            *((void**)(p + idx)) = m_return_data;
            idx += sizeof(void*);
            // edi,dword ptr [ebp+8]
            p[idx++] = 0x8b;
            p[idx++] = 0x7d;
            p[idx++] = 0x08;
            // rep movs
            p[idx++] = 0xf3;
            p[idx++] = 0xa4;
            // eax,dword ptr [ebp+8]
            p[idx++] = 0x8b;
            p[idx++] = 0x45;
            p[idx++] = 0x08;

            p[idx++] = 0x59; // pop ecx
            p[idx++] = 0x5f; // pop edi
            p[idx++] = 0x5e; // pop esi

            p[idx++] = 0x5d; // pop ebp
        }

        // ret
        int ret_len = getRetCode32(m_orig_func, p + idx);
        if(0 == ret_len)
        {
            return false;
        }
        idx += ret_len;
        return true;
    }

    bool genFunction64()
    {
        int idx = 0;
        unsigned char* p = (unsigned char*)m_mocked_func;

        if(E_TYPE_INTEGER == m_return_type || E_TYPE_REFERENCE == m_return_type)
        {
            // mov rax, addr
            p[idx++] = 0x48;
            p[idx++] = 0xb8;
            *((void**)(p + idx)) = (void*)m_return_data;
            idx += sizeof(void*);

            if(E_TYPE_REFERENCE == m_return_type || sizeof(ReturnType) == sizeof(void*))
            {
                // mov (%rax), %rax
                p[idx++] = 0x48;
                p[idx++] = 0x8b;
                p[idx++] = 0x00;
            }
            else
            {// char, short, int
                // mov eax, dword ptr [rax]
                p[idx++] = 0x8b;
                p[idx++] = 0x00;
            }
        }
        else if(E_TYPE_FLOAT == m_return_type)
        {
            // mov eax, addr
            p[idx++] = 0x48;
            p[idx++] = 0xb8;
            *((void**)(p + idx)) = (void*)m_return_data;
            idx += sizeof(void*);

            /*if(sizeof(float) == sizeof(ReturnType))
            {// float
            // fld qword ptr [eax]
            p[idx++] = 0xd9;
            p[idx++] = 0x00;
            }
            else if(sizeof(double) == sizeof(ReturnType))
            {// double
            // fld qword ptr [eax]
            p[idx++] = 0xdd;
            p[idx++] = 0x00;
            }*/
            if(sizeof(float) == sizeof(ReturnType))
            {// float
                // movss %(rax), %xmm0
                p[idx++] = 0xf3;
                p[idx++] = 0x0f;
                p[idx++] = 0x10;
                p[idx++] = 0x00;
            }
            else if(sizeof(double) == sizeof(ReturnType))
            {// double
                // movsd %(rax), %xmm0
                p[idx++] = 0xf2;
                p[idx++] = 0x0f;
                p[idx++] = 0x10;
                p[idx++] = 0x00;
            }
            else if(sizeof(long double) == sizeof(ReturnType))
            {// long double, linux
                // fldt qword ptr [eax]
                p[idx++] = 0xdb;
                p[idx++] = 0x28;
            }
        }
        else if(E_TYPE_STRUCT == m_return_type)
        {
            p[idx++] = 0x55; // push rbp

            // mov %rsp,%rbp
            p[idx++] = 0x48;
            p[idx++] = 0x8b;
            p[idx++] = 0xec;

            p[idx++] = 0x53; // push rbx
            p[idx++] = 0x56; // push esi
            p[idx++] = 0x57; // push edi
            p[idx++] = 0x51; // push ecx
            p[idx++] = 0x52; // push rdx

#if defined(WIN32)
            // mov %rcx, %rbx, save object pointer to %rbx
            p[idx++] = 0x48;
            p[idx++] = 0x89;
            p[idx++] = 0xcb;
#else
            // mov %rdi, %rbx, save object pointer to %rbx
            p[idx++] = 0x48;
            p[idx++] = 0x89;
            p[idx++] = 0xfb;
#endif

            // mov %rax, addr
            p[idx++] = 0x48;
            p[idx++] = 0xb8;
            *((void**)(p + idx)) = (void*)m_return_data;
            idx += sizeof(void*);
            // mov %rbx,%rdx
            p[idx++] = 0x48;
            p[idx++] = 0x89;
            p[idx++] = 0xda;
            // lea sizeof(retValue)(%rax), %rsi
            p[idx++] = 0x48;
            p[idx++] = 0x8d;
            p[idx++] = 0xb0;
            *((unsigned int*)(p + idx)) = sizeof(ReturnType);
            idx += 4;
            // movzbl (%rax),%ecx
            p[idx++] = 0x0f;
            p[idx++] = 0xb6;
            p[idx++] = 0x08;
            // add $0x1,%rax
            p[idx++] = 0x48;
            p[idx++] = 0x83;
            p[idx++] = 0xc0;
            p[idx++] = 0x01;
            // mov %cl,(%rdx)
            p[idx++] = 0x88;
            p[idx++] = 0x0a;
            // add $0x1,%rdx
            p[idx++] = 0x48;
            p[idx++] = 0x83;
            p[idx++] = 0xc2;
            p[idx++] = 0x01;
            // cmp %rsi,%rax
            p[idx++] = 0x48;
            p[idx++] = 0x39;
            p[idx++] = 0xf0;
            // jne ee
            p[idx++] = 0x75;
            p[idx++] = 0xee;

            // mov %rbx, %rax
            p[idx++] = 0x48;
            p[idx++] = 0x89;
            p[idx++] = 0xd8;

            p[idx++] = 0x5a; // pop rdx
            p[idx++] = 0x59; // pop ecx
            p[idx++] = 0x5f; // pop edi
            p[idx++] = 0x5e; // pop esi
            p[idx++] = 0x5b; // pop rbx

            p[idx++] = 0x5d; // pop ebp
        }

        // ret
        int ret_len = getRetCode32(m_orig_func, p + idx);
        if(0 == ret_len)
        {
            return false;
        }
        idx += ret_len;
        return true;
    }

public:
    static int getPageSize()
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

    bool setReturnValue(ReturnType retValue)
    {
        if(NULL == m_orig_func)
            return false;

        if(E_TYPE_REFERENCE != m_return_type)
            memcpy((unsigned char*)m_return_data, &retValue, sizeof(ReturnType));
        else
        {
            void* v = &retValue;
            memcpy((unsigned char*)m_return_data, &v, sizeof(v));
        }

        return true;
    }

    bool mock(void* func)
    {
        if(NULL != m_orig_func)
        {// already mocked
            return func == m_orig_func;
        }

        m_orig_func = func;

#define MOCK_CODE_SIZE	64
        int sz = MOCK_CODE_SIZE + (E_TYPE_REFERENCE == m_return_type?sizeof(void*):sizeof(ReturnType));
#if defined(WIN32)
        m_mocked_func = malloc(sz); // VirtualAlloc(NULL, sz, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        DWORD old_protect = 0;
        VirtualProtect(m_mocked_func, MOCK_CODE_SIZE, PAGE_EXECUTE_READWRITE, &old_protect);
#elif defined(LINUX)
        int page_size = getPageSize();
        sz = (sz + page_size - 1)/page_size*page_size;
        m_mocked_func = mmap(0, sz, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#elif defined(MACOS)
#else
#endif
        m_alloc_size = sz;
        m_return_data = (unsigned char*)m_mocked_func + MOCK_CODE_SIZE; // align to 4 bytes

        bool ret;
        if(osBit64())
            ret = genFunction64();
        else
            ret = genFunction32();
        if(!ret)
        {
            printf("failed to generate function\n");
            m_orig_func = NULL;
            cleanup();
            return false;
        }
        fnMock(m_orig_func, m_mocked_func);
        return true;
    }

    void unmock()
    {
        if(NULL != m_orig_func)
        {
            fnUnmock(m_orig_func);
            m_orig_func = NULL;
        }
        cleanup();
    }
};

#endif
