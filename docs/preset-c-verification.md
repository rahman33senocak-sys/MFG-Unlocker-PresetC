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
