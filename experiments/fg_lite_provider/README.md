# Experimental OptiScaler DLSS-G Preset C Lite provider

NOT a rebuilt MFGUnlocker.asi. This is a standalone DX12 NGX passthrough DLL for OptiScaler custom/PresetCLite/dlssg.
The original MFGUnlocker.asi remains active for SM86 RTX30 kernel translation, DLSS5 and x6 control.
It forwards all DX12 NGX entry points to the real NVIDIA provider from PresetCLite.ini and
sets DLSSG.LiteCandidateBlendingEnabled = 1 and DLSSG.UserInterfaceRecompositionEnabled = 0 before FG CreateFeature.
It does not detour or patch MFGUnlocker or NVIDIA's provider memory.

INSTALL / 007 FIRST LIGHT
1. Back up OptiScaler.ini and your MFGUnlocker.asi.
2. Extract custom/PresetCLite/dlssg/nvngx_dlssg.dll and PresetCLite.ini next to your OptiScaler main DLL.
3. Edit PresetCLite.ini Path= to match the exact activeProviderPath from RTXMFG-Universal.status.json;
   shipped value matches the supplied 007 First Light provider. Do not copy or redistribute NVIDIA binaries.
4. Keep the ORIGINAL MFGUnlocker.asi in OptiScaler/plugins and Preset C selected (dlssgPreset=3).
5. Under [FrameGen] in OptiScaler.ini set:
     Enabled=true
     FGInput=dlssg
     FGOutput=dlssg
     FGNvngxReplacement=PresetCLite
6. Start the game. Look for custom/PresetCLite/dlssg/PresetCLite-proxy.log
   and PRESET_C_LITE_CREATE getBefore=... readAfter=0/1,0/0.
7. This only validates NGX parameter readback; FG Lite CUDA dispatch and any FPS benefit remain unverified.

LIMITATIONS / ROLLBACK
- DX12 only, not Vulkan. Actual NVIDIA FG operation requires original MFGUnlocker to remain functional.
- This proxy is experimental, not yet validated on RTX 30 and might fail feature creation.
- Setting this replacement forces Lite for every DX12 FG Create. For Preset A tests revert to None.
- If FG fails, revert FGNvngxReplacement=None and remove the custom folder.
- Do not use with anti-cheat protected games.
- This does not produce or replace the complete MFG-Unlocker v1.7.2-beta.2 DLL.

BUILD
GitHub Actions compiles with Windows MSVC2022; official NVIDIA Streamline NGX headers
are downloaded as compile dependencies and not included in the distributed ZIP.
