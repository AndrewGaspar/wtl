#pragma once

#include <Windows.h>
#include <SetupAPI.h>
#include <utility>

#include "resource_handle.h"
#include "result.h"

namespace wtl
{
    using hdevinfo = resource_handle<HDEVINFO, int, -1, decltype(::SetupDiDestroyDeviceInfoList), ::SetupDiDestroyDeviceInfoList>;

    namespace setup_di
    {
        inline win32_err_t<hdevinfo> create_devinfo_info_list(GUID const & classGuid, HWND hwndParent = NULL, PCWSTR machineName = NULL)
        {
            hdevinfo hdo = SetupDiCreateDeviceInfoListExW(&classGuid, hwndParent, machineName, nullptr);
            if (!hdo) return GetLastError();

            return win32_err_t<hdevinfo>::success(std::move(hdo));
        }

        inline win32_err_t<hdevinfo> create_devinfo_info_list(HWND hwndParent = NULL, PCWSTR machineName = NULL)
        {
            hdevinfo hdo = SetupDiCreateDeviceInfoListExW(nullptr, hwndParent, machineName, nullptr);
            if (!hdo) return GetLastError();

            return win32_err_t<hdevinfo>::success(std::move(hdo));
        }

        inline win32_err_t<SP_DEVINFO_DATA> create_device_info(
            _In_ HDEVINFO deviceInfoSet,
            _In_ PCWSTR deviceName,
            _In_ GUID const & setupClass,
            _In_opt_ PCWSTR deviceDescription = nullptr,
            _In_opt_ HWND hwndParent = nullptr,
            _In_ DWORD creationFlags = 0)
        {
            SP_DEVINFO_DATA devInfoData = { sizeof(SP_DEVINFO_DATA) };
            if (!
                SetupDiCreateDeviceInfoW(
                    deviceInfoSet, 
                    deviceName, 
                    &setupClass, 
                    deviceDescription, 
                    hwndParent, 
                    creationFlags, 
                    &devInfoData))
            {
                return GetLastError();
            }

            return win32_err_t<SP_DEVINFO_DATA>::success(std::move(devInfoData));
        }

        inline win32_err_t<hdevinfo> get_class_devs(
            _In_ GUID const & classGuid,
            _In_opt_ PCWSTR enumerator = nullptr,
            _In_opt_ HWND hwndParent = nullptr,
            _In_ DWORD flags = 0,
            _In_opt_ PCWSTR machineName = nullptr)
        {
            hdevinfo hdo = SetupDiGetClassDevsExW(&classGuid, enumerator, hwndParent, flags, nullptr, machineName, nullptr);
            if (!hdo) return GetLastError();

            return win32_err_t<hdevinfo>::success(std::move(hdo));
        }

        inline win32_err_t<SP_DEVINFO_DATA> enum_device_info(_In_ HDEVINFO deviceInfoSet, _In_ DWORD memberIndex)
        {
            SP_DEVINFO_DATA devInfoData = { sizeof(SP_DEVINFO_DATA) };
            if (!SetupDiEnumDeviceInfo(deviceInfoSet, memberIndex, &devInfoData))
            {
                return GetLastError();
            }

            return win32_err_t<SP_DEVINFO_DATA>::success(std::move(devInfoData));
        }
    }
}