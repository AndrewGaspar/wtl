#pragma once

#include "resource_handle.h"
#include <winsvc.h>

namespace wtl
{
    using sc_handle = resource_handle<SC_HANDLE, int, 0, decltype(::CloseServiceHandle), ::CloseServiceHandle>;
}