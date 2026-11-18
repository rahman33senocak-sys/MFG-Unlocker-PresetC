#!/usr/bin/env python3
"""Compile the actual Ampere Create-boundary function with Windows SDK mocks.

This checks the new function's MSVC syntax/SEH behavior without asserting
that a full CUDA/NGX/Streamline DLL can be linked or run.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "source/native/ampere_backend.cpp").read_text(encoding="utf-8")
start_marker = "void PinPresetCLiteAtCreate("
end_marker = "void ObserveCreateDevice("
assert source.count(start_marker) == 1
start = source.index(start_marker)
end = source.index(end_marker, start)
fragment = source[start:end]

header = r"""
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cstdint>
#include <cwchar>
#include <map>
#include <string>
#include <cassert>
#include "../../source/native/dlssg_fg_lite_params.h"

using NVSDK_NGX_Result = int;
constexpr NVSDK_NGX_Result NVSDK_NGX_Result_Success = 0;

struct NVSDK_NGX_Parameter {
    std::map<std::string, uint32_t> values;
    NVSDK_NGX_Result Get(const char* key, uint32_t* output) const {
        const auto it = values.find(key);
        if (it == values.end()) return 1;
        *output = it->second;
        return 0;
    }
    void Set(const char* key, uint32_t value) { values[key] = value; }
};
namespace ampere_gpu {
    bool ready = true;
    bool FgLiteKernelPrepared() noexcept { return ready; }
}
namespace dlssg_preset {
    struct SelectionSnapshot {
        uint32_t requested = 3, latched = 3;
        bool restartRequired = false;
    };
    SelectionSnapshot selected{};
    SelectionSnapshot ReadSelection() noexcept { return selected; }
}
std::wstring lastLog;
void Report(const wchar_t* line) noexcept { lastLog = line; }
"""

footer = r"""
int main() {
    NVSDK_NGX_Parameter params{};
    params.Set(dlssg_fg_lite_params::kLite, 0);
    params.Set(dlssg_fg_lite_params::kUiRecomposition, 1);
    PinPresetCLiteAtCreate(&params, true, L"D3D12");
    assert(params.values.at(dlssg_fg_lite_params::kLite) == 1);
    assert(params.values.at(dlssg_fg_lite_params::kUiRecomposition) == 0);
    assert(lastLog.find(L"pinned=1") != std::wstring::npos);

    params.Set(dlssg_fg_lite_params::kLite, 0);
    dlssg_preset::selected.requested = 1;
    dlssg_preset::selected.latched = 1;
    PinPresetCLiteAtCreate(&params, true, L"D3D12");
    assert(params.values.at(dlssg_fg_lite_params::kLite) == 0);

    dlssg_preset::selected = {};
    ampere_gpu::ready = false;
    PinPresetCLiteAtCreate(&params, true, L"Vulkan");
    assert(params.values.at(dlssg_fg_lite_params::kLite) == 0);

    ampere_gpu::ready = true;
    PinPresetCLiteAtCreate(&params, false, L"Vulkan");
    assert(params.values.at(dlssg_fg_lite_params::kLite) == 0);
    return 0;
}
"""

out = Path(__file__).with_name("generated_create_boundary_smoke.cpp")
out.write_text(header + fragment + footer, encoding="utf-8")
print(out)
