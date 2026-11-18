# Preset C investigation — RTX 30 / 007 First Light

This branch adds **evidence**, not a claimed FG Lite performance fix.

## What the original 1.7.1 code already does
- Supports DLSS-G 310.9.16 and maps the Preset C DRS value to 3.
- Validates its **26 named kernel slots**, with `BlendCandidatesFGLite` at slot 1.
- Validates and publishes a complete SM86 native or PTX program before an FG Create.
- Records the preset observed in the provider's configuration read.
- Can restart and re-latch the provider configuration after a preset change.

## Added by this branch

`ampereFgLiteKernelPrepared` in `RTXMFG-Universal.status.json` is true only if:
1. The **bound**, not merely discovered, provider has the 310.9.16/26-slot profile;
2. Both the ordinary blend slot and the FG Lite slot have published replacement descriptors;
3. The complete Ampere program is marked ready.

The in-game **Debug → Ampere** section displays this separately from the
`dlssgPresetObserved` readback.

**Important:** A published kernel and a recorded Preset C read do *not*
prove that `BlendCandidatesFGLite` was launched by the CUDA driver.
Neither field measures GPU time, GPU utilization, or delivered FPS.

## Experimental NGX FG Lite pin (this branch)

The upstream 310.9.16 DRS preset value `3` is already observed in the
007 First Light status snapshot; it does **not** prove that the separate
`DLSSG.LiteCandidateBlendingEnabled` feature parameter reached the runtime.

This branch adds a narrow Create-boundary experiment in
`ampere_backend.cpp`, for D3D12 and Vulkan:

- Only a verified, prepared 26-slot Ampere program with the
  `BlendCandidatesFGLite` slot can enter the helper.
- The selection must be requested and latched at **3**, with no pending
  restart; other presets, DLSS SR/RR and DLSS 5 are left untouched.
- At FG Create (not every Evaluate), the NGX parameter map receives
  `DLSSG.LiteCandidateBlendingEnabled=1` and
  `DLSSG.UserInterfaceRecompositionEnabled=0`.
- The helper reads both keys back. A log record beginning
  `PRESET_C_LITE_CREATE` reports `pinned=1` only when both readbacks
  match; `pinned=0` or `exception=1` means the parameters could not
  be verified. With no record, the guarded path was not entered.
- The key may be encountered once in the runtime Create and again in its
  nested provider Create. Re-pinning is intentionally idempotent.

**What this cannot prove:** that the NVIDIA runtime actually selects the
FG Lite CUDA kernel, that the player's overlay changes from "Full" to
"Lite", or that GPU cost/FPS improves. A Create-parameter readback is
not a CUDA launch trace. If `pinned=1` but the display still says
"Full", compare GPU-side kernel launches, not the NVAPI preset menu value.
Please keep the original `.asi` backed up and do not assume the debug
build is production-safe.

### Portable checks

The workflow runs both Python status-regression tests and a portable C++
fake NGX parameter test (`tests/preset_c_validation/fg_lite_params_test.cpp`).
These check policy, readback, absent keys, and idempotence. They do **not**
compile `ampere_backend.cpp` under MSVC or execute GPU kernels. A validated
Windows build and on-device A/C comparison remain required.

## Read a captured status file

```powershell
py -3 tools/preset_c_verify.py "C:\\path\\to\\RTXMFG-Universal.status.json"
py -3 tools/preset_c_verify.py "C:\\path\\to\\RTXMFG-Universal.status.json" --json
```

For an upstream 1.7.1 status snapshot that lacks the new kernel field,
the tool reports `preset-c-consumed-legacy-build-no-kernel-evidence`, not a
false failure or a false success. Portable regression tests:

```bash
python -m unittest discover -s tests/preset_c_validation -p "test_*.py"
```

## Reproduce a performance difference

1. Use the *same* actual DLSS-G provider (310.9.16), game settings, resolution,
   driver, x6 mode, game location and OptiScaler configuration for A/B/C.
2. Restart the game between each preset where hot switching cannot be verified.
3. Disable FPS caps, V-Sync and dynamic FPS targeting for the uncapped comparison.
   Record real/input FPS separately from generated/output FPS.
4. Run at least three stable samples per preset; compare median FPS, GPU frame
   timing and 1% lows rather than a single instantaneous FPS value.
5. Measure GPU FG timing or identify `BlendCandidatesFGLite` launches via GPU
   profiling to distinguish a real execution-path failure from a CPU/pacing/cap
   bottleneck. Higher generated-FPS readings do not prove a better GPU FG kernel.

### Known example (007 First Light)

The supplied v1.7.1 status showed provider 310.9.16, requested/latched/
observed Preset 3, nine override reads, native cache loaded, working x6.
This establishes successful **configuration readback**. It does **not** establish
the absence or presence of a Preset C GPU speedup.

## Build limitation

Upstream `source/native/build.ps1` requires a pinned Windows MSVC/SDK/CUDA
toolchain, third-party headers, and **private validated native kernel-cache
inputs**. Public source checkout by itself cannot reproduce the release DLL.
Do not publish a placeholder DLL or mix unvalidated kernels into a release.
