#!/usr/bin/env python3
"""Explain Preset C evidence from an RTXMFG status snapshot.

No GPU dispatch or performance claim is made from configuration readback.
This is intentionally usable with both upstream v1.7.1 and diagnostic builds.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


def inspect_status(data: dict[str, Any]) -> dict[str, Any]:
    requested = data.get("dlssgPresetRequested")
    latched = data.get("dlssgPresetLatched")
    observed = data.get("dlssgPresetObserved")
    read_count = data.get("dlssgPresetOverrideReadCount", 0)
    version = tuple(data.get(k, 0) for k in (
        "activeProviderVersionMajor", "activeProviderVersionMinor",
        "activeProviderVersionBuild"))
    ampere = data.get("gpuFamily") in (2, 4)
    available = data.get("dlssgPresetCAvailable") is True
    selected = requested == 3 and latched == 3
    consumed = (selected and data.get("dlssgPresetObservedValid") is True
                and observed == 3 and isinstance(read_count, int)
                and read_count > 0)
    lite = data.get("ampereFgLiteKernelPrepared")
    gpu_work = data.get("ampereEvaluations", 0)
    if requested != 3:
        outcome = "preset-c-not-requested"
    elif latched != 3:
        outcome = "selection-pending-or-restart-required"
    elif version != (310, 9, 16) or not available:
        outcome = "provider-c-not-supported-or-not-selected"
    elif not consumed:
        outcome = "provider-configuration-read-not-confirmed"
    elif ampere and data.get("ampereProgramReadyMfg") is not True:
        outcome = "ampere-program-not-ready"
    elif ampere and lite is False:
        outcome = "fg-lite-program-not-published"
    elif ampere and lite is None:
        outcome = "preset-c-consumed-legacy-build-no-kernel-evidence"
    elif ampere and (not isinstance(gpu_work, int) or gpu_work == 0):
        outcome = "prepared-no-fg-evaluations-observed"
    else:
        outcome = "preset-c-config-and-program-prepared-gpu-unverified"
    return {
        "outcome": outcome,
        "requested": requested,
        "latched": latched,
        "provider_version": ".".join(map(str, version)),
        "provider_reports_preset_c": available,
        "provider_configuration_consumed_c": consumed,
        "ampere_route": ampere,
        "fg_lite_program_prepared": lite if isinstance(lite, bool) else None,
        "kernel_image": data.get("ampereKernelImage"),
        "native_cache_status": data.get("ampereNativeCacheStatus"),
        "fg_evaluations": gpu_work,
        "generated_frames_requested": data.get("ampereLastCreateCount"),
        "gpu_lite_dispatch_verified": False,
        "performance_improvement_verified": False,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("status", type=Path, help="RTXMFG-Universal.status.json")
    parser.add_argument("--json", action="store_true", help="Output machine-readable diagnostics")
    args = parser.parse_args()
    try:
        data = json.loads(args.status.read_text(encoding="utf-8-sig"))
        if not isinstance(data, dict):
            raise ValueError("the status JSON root must be an object")
        report = inspect_status(data)
    except (OSError, UnicodeError, ValueError) as exc:
        parser.exit(2, f"error: {exc}\n")
    if args.json:
        print(json.dumps(report, indent=2, ensure_ascii=False))
    else:
        for key, value in report.items():
            print(f"{key}: {value}")
        print("Note: Preset C consumption and kernel publication are not GPU dispatch measurements.")
        print("To demonstrate a speed benefit, compare controlled FG GPU timings with A/B/C.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
