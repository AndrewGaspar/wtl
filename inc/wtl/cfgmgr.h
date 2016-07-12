#pragma once

#include <Windows.h>
#include <cfgmgr32.h>

#include "resource_handle.h"
#include "result.h"
#include "multi_sz.h"

#define RETURN_IF_NOT_CR_SUCCESS(_crExpression) { \
    auto _wtl_result = _crExpression; \
    if (_wtl_result != CR_SUCCESS) \
    { \
        return _wtl_result; \
    } \
} REQUIRE_SEMICOLON

namespace wtl
{
    namespace cm
    {
        static configret_t<ULONG> get_device_interface_list_size(GUID const & classGuid, PCWSTR pDeviceId = nullptr, ULONG flags = 0)
        {
            ULONG size;
            RETURN_IF_NOT_CR_SUCCESS(CM_Get_Device_Interface_List_SizeW(&size, (LPGUID)&classGuid, (DEVINSTID_W)pDeviceId, flags));

            return configret_t<ULONG>::success(size);
        }

        static configret_t<wtl::multi_sz> get_device_interface_list(GUID const & classGuid, PCWSTR pDeviceId = nullptr, ULONG flags = 0)
        {
            RETURN_OR_UNWRAP(size, get_device_interface_list_size(classGuid, pDeviceId, flags));

            std::vector<wchar_t> buffer(size, L'\0');

            RETURN_IF_NOT_CR_SUCCESS(CM_Get_Device_Interface_ListW((LPGUID)&classGuid, (DEVINSTID_W)pDeviceId, &buffer[0], size, flags));

            return configret_t<wtl::multi_sz>::success(std::move(buffer));
        }

        static DWORD map_configret_to_win32_err(CONFIGRET cr, DWORD defaultError = ERROR_INVALID_FUNCTION)
        {
            return CM_MapCrToWin32Err(cr, defaultError);
        }
    }
}