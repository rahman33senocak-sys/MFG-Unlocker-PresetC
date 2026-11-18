"""Regression tests for the portable Preset C status interpreter."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from preset_c_verify import inspect_status


def sample(**overrides):
    result = dict(
        dlssgPresetRequested=3, dlssgPresetLatched=3,
        dlssgPresetObserved=3, dlssgPresetObservedValid=True,
        dlssgPresetOverrideReadCount=9, dlssgPresetCAvailable=True,
        activeProviderVersionMajor=310, activeProviderVersionMinor=9,
        activeProviderVersionBuild=16, gpuFamily=2,
        ampereProgramReadyMfg=True, ampereFgLiteKernelPrepared=True,
        ampereKernelImage=2, ampereNativeCacheStatus=1,
        ampereEvaluations=100, ampereLastCreateCount=5)
    result.update(overrides)
    return result


class TestPresetCStatus(unittest.TestCase):
    def test_full_prepared_without_gpu_proof(self):
        result = inspect_status(sample())
        self.assertEqual(result["outcome"],
                         "preset-c-config-and-program-prepared-gpu-unverified")
        self.assertTrue(result["provider_configuration_consumed_c"])
        self.assertTrue(result["fg_lite_program_prepared"])
        self.assertFalse(result["gpu_lite_dispatch_verified"])
        self.assertFalse(result["performance_improvement_verified"])

    def test_legacy_status_must_not_imply_kernel(self):
        old = sample()
        old.pop("ampereFgLiteKernelPrepared")
        self.assertEqual(inspect_status(old)["outcome"],
                         "preset-c-consumed-legacy-build-no-kernel-evidence")

    def test_missing_lite_program(self):
        self.assertEqual(inspect_status(sample(ampereFgLiteKernelPrepared=False))["outcome"],
                         "fg-lite-program-not-published")

    def test_c_requested_b_latched(self):
        self.assertEqual(inspect_status(sample(dlssgPresetLatched=2))["outcome"],
                         "selection-pending-or-restart-required")

    def test_c_not_consumed(self):
        self.assertEqual(inspect_status(sample(dlssgPresetObserved=2))["outcome"],
                         "provider-configuration-read-not-confirmed")

    def test_non_c_provider(self):
        self.assertEqual(inspect_status(sample(activeProviderVersionBuild=1))["outcome"],
                         "provider-c-not-supported-or-not-selected")

    def test_not_selected(self):
        self.assertEqual(inspect_status(sample(dlssgPresetRequested=1))["outcome"],
                         "preset-c-not-requested")


if __name__ == "__main__":
    unittest.main()
