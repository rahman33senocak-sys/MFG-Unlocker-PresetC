#pragma once

#include <cstdint>

// 310.9.16 has two independent ways of selecting FG Lite: the NVAPI
// render-preset DRS value and the NGX feature parameter below. The former
// alone does not prove that the feature took the lightweight execution path.
// Only a fully published 26-slot Ampere program is eligible for the pin.
namespace dlssg_fg_lite_params
{
inline constexpr char kLite[] = "DLSSG.LiteCandidateBlendingEnabled";
inline constexpr char kUiRecomposition[] = "DLSSG.UserInterfaceRecompositionEnabled";

constexpr bool ShouldPin(bool ampereProgramWithLitePrepared,
    uint32_t requested, uint32_t latched, bool restartRequired) noexcept
{
    return ampereProgramWithLitePrepared && !restartRequired
        && requested == 3 && latched == 3;
}

struct Readback
{
    bool liteBeforeValid = false, uiBeforeValid = false;
    uint32_t liteBefore = 0, uiBefore = 0;
    bool liteAfterValid = false, uiAfterValid = false;
    uint32_t liteAfter = 0, uiAfter = 0;
    constexpr bool Pinned() const noexcept
    { return liteAfterValid && liteAfter == 1 && uiAfterValid && uiAfter == 0; }
};

// NGX Parameter has Get(name, uint32_t*) and Set(name, uint32_t). The helper
// is templated so tests can use a fake parameter object without NVIDIA SDK.
// Caller must have established an exact frame-generation Create on RTX 30,
// a valid selected 310.9.16 provider program and mutable NGX parameters.
template<class Parameters, class Success>
Readback Pin(Parameters& parameters, Success success) noexcept
{
    Readback result{};
    result.liteBeforeValid = parameters.Get(kLite, &result.liteBefore) == success;
    result.uiBeforeValid = parameters.Get(kUiRecomposition, &result.uiBefore) == success;
    parameters.Set(kLite, static_cast<uint32_t>(1));
    parameters.Set(kUiRecomposition, static_cast<uint32_t>(0));
    result.liteAfterValid = parameters.Get(kLite, &result.liteAfter) == success;
    result.uiAfterValid = parameters.Get(kUiRecomposition, &result.uiAfter) == success;
    return result;
}
} // namespace dlssg_fg_lite_params
