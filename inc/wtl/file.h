#pragma once

#include <windows.h>

#include "resource_handle.h"
#include "result.h"
#include "primitives.h"

namespace wtl
{
    class overlapped
    {
        OVERLAPPED ol = {};
    public:
        overlapped(std::uint32_t offset = 0, std::uint32_t offsetHigh = 0, HANDLE event = NULL)
        {
            ol.Offset = 0;
            ol.OffsetHigh = offsetHigh;
            ol.hEvent = event;
        }

        explicit overlapped(HANDLE event) : overlapped(0, 0, event) { }

        LPOVERLAPPED get() { return &ol; }

        win32_err_t<DWORD> get_num_bytes_read(HANDLE file, dword_milliseconds timeout = infinite, bool alertable = false)
        {
            DWORD bytesTransferred;
            if (!::GetOverlappedResultEx(file, get(), &bytesTransferred, timeout.count(), alertable)) 
                return GetLastError();

            return win32_err_t<DWORD>::success(bytesTransferred);
        }
    };

    class file : public handle
    {
        template<typename T>
        T * addressof(T& ref) { return &ref; }
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

        template<typename It>
        win32_err read(It begin, It end, _In_ DWORD * bytesRead = nullptr, _In_ LPOVERLAPPED overlapped = nullptr)
        {
            static_assert(std::is_same<std::random_access_iterator_tag, std::iterator_traits<It>::iterator_category>::value,
                "file::read must provide random access iterators.");

            auto size = (end - begin) * sizeof(decltype(*begin));
            if (!::ReadFile(get(), (LPVOID)addressof(*begin), static_cast<DWORD>(size), bytesRead, overlapped)) return GetLastError();

            return ERROR_SUCCESS;
        }

        template<typename It>
        win32_err read(It begin, It end, _In_ LPOVERLAPPED overlapped)
        {
            return read(begin, end, nullptr, overlapped);
        }

        win32_err cancel(LPOVERLAPPED overlapped = nullptr)
        {
            if (!::CancelIoEx(get(), overlapped)) return GetLastError();

            return ERROR_SUCCESS;
        }
    };
}