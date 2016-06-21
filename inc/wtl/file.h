#pragma once

#include <windows.h>

#include "resource_handle.h"
#include "result.h"

namespace wtl
{
    class file : public handle
    {
    public:
        file() : handle() { }

        file(HANDLE h) : handle(h) { }

        static win32_err_t<file> create(
            _In_     PCWSTR fileName, 
                     DWORD desiredAccess,
                     DWORD shareMode = 0,
            _In_opt_ LPSECURITY_ATTRIBUTES securityAttributes = nullptr,
                     DWORD creationDisposition = OPEN_EXISTING,
                     DWORD flagsAndAttributes = FILE_ATTRIBUTE_NORMAL,
            _In_opt_ HANDLE templateFile = nullptr)
        {
            auto handle = file(CreateFileW(
                fileName,
                desiredAccess,
                shareMode,
                securityAttributes,
                creationDisposition,
                flagsAndAttributes,
                templateFile));

            if (!handle)
            {
                return GetLastError();
            }

            return win32_err_t<file>::success(std::move(handle));
        }
    };
}