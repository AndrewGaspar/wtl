#pragma once

#include <Synchapi.h>

#include "primitives.h"
#include "resource_handle.h"

namespace wtl
{
    enum class wait_result : DWORD
    {
        abandoned = WAIT_ABANDONED,
        io_completion = WAIT_IO_COMPLETION,
        signaled = WAIT_OBJECT_0,
        timeout = WAIT_TIMEOUT,
        failed = WAIT_FAILED
    };

    class event : public resource_handle<HANDLE, int, NULL, decltype(::CloseHandle), ::CloseHandle>
    {
    public:
        event() : resource_handle() { }

        event(HANDLE h) : resource_handle(h) { }

        static win32_err_t<event> create(bool bInitialState = false, bool bManualReset = false, PCWSTR lpName = nullptr, LPSECURITY_ATTRIBUTES lpEventAttributes = nullptr)
        {
            event newEvent = ::CreateEventW(lpEventAttributes, bManualReset, bInitialState, lpName);
            if (!newEvent) return GetLastError();

            return win32_err_t<event>::success(std::move(newEvent));
        }

        win32_err set()
        {
            if (!::SetEvent(get())) return GetLastError();

            return ERROR_SUCCESS;
        }

        win32_err reset()
        {
            if (!::ResetEvent(get())) return GetLastError();

            return ERROR_SUCCESS;
        }

        win32_err_t<wait_result> wait(
            dword_milliseconds timeout = infinite,
            bool bAlertable = false)
        {
            auto result = static_cast<wait_result>(::WaitForSingleObjectEx(get(), timeout.count(), bAlertable));
            if (result == wait_result::failed) return GetLastError();

            return win32_err_t<wait_result>::success(result);
        }
    };

    namespace details
    {
        template<typename T>
        HANDLE get(T const & handle) { return handle.get(); }
    }

    template<typename... THandleArgs>
    win32_err_t<HANDLE> wait_for_multiple_objects(
        bool waitAll,
        std::chrono::duration<DWORD, std::milli> timeout,
        bool alertable,
        THandleArgs const &... handleObjects)
    {
        HANDLE handles[] = { details::get(handleObjects)... };

        const auto count = sizeof(handles) / sizeof(HANDLE);

        auto result = ::WaitForMultipleObjectsEx(count, handles, waitAll, timeout.count(), alertable);

        if (result < WAIT_OBJECT_0 || result >= (WAIT_OBJECT_0 + count))
            return GetLastError();

        return win32_err_t<HANDLE>::success(handles[result - WAIT_OBJECT_0]);
    }

    template<typename... THandleArgs>
    win32_err_t<HANDLE> wait_for_all_objects(
        THandleArgs const &... handleObjects)
    {
        return wait_for_multiple_objects(true, infinite, false, handleObjects...);
    }

    template<typename... THandleArgs>
    win32_err_t<HANDLE> wait_for_any_object(THandleArgs const &... handleObjects)
    {
        return wait_for_multiple_objects(false, infinite, false, handleObjects...);
    }
}