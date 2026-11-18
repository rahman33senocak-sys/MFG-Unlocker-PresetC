// Experimental OptiScaler custom DLSS-G provider for 007 First Light (DX12).
// Independent proxy: no code-patching or hooks into MFGUnlocker.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <nvsdk_ngx_params.h>
#include <nvsdk_ngx_defs.h>
#include <cstdint>
#include <cwchar>
#include <mutex>
#include <string>

namespace {
HMODULE selfModule = nullptr;
HMODULE nativeProvider = nullptr;
std::once_flag loadOnce;
std::wstring moduleDirectory;
constexpr uint32_t kFail = 0xBAD00001u;
constexpr const wchar_t* kDefaultProvider =
    L"C:\\ProgramData\\NVIDIA\\NGX\\models\\dlssg\\versions\\20318480\\files\\160_E658700.bin";

std::wstring SelfDirectory() {
    wchar_t path[32768]{};
    DWORD n = GetModuleFileNameW(selfModule, path, 32768);
    std::wstring result(path, n);
    auto pos = result.find_last_of(L"\\/");
    return pos == std::wstring::npos ? L"" : result.substr(0, pos + 1);
}
void Log(const wchar_t* message) {
    OutputDebugStringW(message);
    if (moduleDirectory.empty()) return;
    const auto file = moduleDirectory + L"PresetCLite-proxy.log";
    HANDLE handle = CreateFileW(file.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return;
    SYSTEMTIME now{}; GetLocalTime(&now);
    char timestamp[128]{};
    wsprintfA(timestamp, "[%02u:%02u:%02u] ",now.wHour,now.wMinute,now.wSecond);
    DWORD written = 0;
    WriteFile(handle, timestamp, lstrlenA(timestamp), &written, nullptr);
    char messageUtf8[1400]{};
    int length = WideCharToMultiByte(CP_UTF8,0,message,-1,messageUtf8,sizeof(messageUtf8),nullptr,nullptr);
    if (length>0) WriteFile(handle,messageUtf8,length-1,&written,nullptr);
    WriteFile(handle,"\r\n",2,&written,nullptr);
    CloseHandle(handle);
}
void InitializeNative() {
    moduleDirectory = SelfDirectory();
    auto ini = moduleDirectory + L"PresetCLite.ini";
    wchar_t configured[32768]{};
    GetPrivateProfileStringW(L"Provider",L"Path",kDefaultProvider,configured,32768,ini.c_str());
    if (!configured[0]) {Log(L"ERROR: No NVIDIA provider path");return;}
    wchar_t ownPath[32768]{};
    GetModuleFileNameW(selfModule,ownPath,32768);
    if (_wcsicmp(ownPath,configured)==0) {Log(L"ERROR: Provider path recursively points to proxy");return;}
    DWORD attr = GetFileAttributesW(configured);
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        Log(L"ERROR: NVIDIA provider file not found; edit PresetCLite.ini");
        return;
    }
    nativeProvider = GetModuleHandleW(configured);
    if (!nativeProvider) nativeProvider = LoadLibraryW(configured);
    if (nativeProvider) {
        Log(L"Loaded real NVIDIA provider (MFGUnlocker handles the original RTX 30/SM86 route)");
    } else Log(L"ERROR: Could not load NVIDIA provider; check path/version");
}
FARPROC Export(const char* name) {
    std::call_once(loadOnce,InitializeNative);
    return nativeProvider ? GetProcAddress(nativeProvider,name) : nullptr;
}
template<class Fn> Fn Resolve(const char* name) {
    return reinterpret_cast<Fn>(Export(name));
}
void PinLite(NVSDK_NGX_Parameter* p) {
    if (!p) return;
    // Separate from the DRS Preset C selection: these are NGX feature params.
    uint32_t liteBefore=0,uiBefore=0,liteAfter=0,uiAfter=0;
    auto a=p->Get("DLSSG.LiteCandidateBlendingEnabled",&liteBefore);
    auto b=p->Get("DLSSG.UserInterfaceRecompositionEnabled",&uiBefore);
    p->Set("DLSSG.LiteCandidateBlendingEnabled",static_cast<unsigned int>(1));
    p->Set("DLSSG.UserInterfaceRecompositionEnabled",static_cast<unsigned int>(0));
    auto c=p->Get("DLSSG.LiteCandidateBlendingEnabled",&liteAfter);
    auto d=p->Get("DLSSG.UserInterfaceRecompositionEnabled",&uiAfter);
    wchar_t line[320]{};
    wsprintfW(line,L"PRESET_C_LITE_CREATE getBefore=%u/%u,%u/%u readAfter=%u/%u,%u/%u",
        static_cast<uint32_t>(a),liteBefore,static_cast<uint32_t>(b),uiBefore,
        static_cast<uint32_t>(c),liteAfter,static_cast<uint32_t>(d),uiAfter);
    Log(line);
}
}
extern "C" BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH){selfModule=h;DisableThreadLibraryCalls(h);}
    return TRUE;
}

#define FORWARD(NAME,ARGS,CALL) \
extern "C" __declspec(dllexport) uint32_t __cdecl NVSDK_NGX_##NAME ARGS { \
    using F = uint32_t(__cdecl*) ARGS; \
    auto fn=Resolve<F>("NVSDK_NGX_" #NAME); \
    return fn ? fn CALL : kFail; \
}

// DX12 only. These typed exports match the NGX D3D12 ABI on Windows x64.
FORWARD(D3D12_Init,(uint64_t id,const wchar_t* dir,void* device,const void* info,uint32_t ver),(id,dir,device,info,ver))
FORWARD(D3D12_Init_Ext,(uint64_t id,const wchar_t* dir,void* device,uint32_t ver,const void* info),(id,dir,device,ver,info))
FORWARD(D3D12_Shutdown,(),())
FORWARD(D3D12_Shutdown1,(void* device),(device))
FORWARD(D3D12_GetScratchBufferSize,(uint32_t feature,const NVSDK_NGX_Parameter* p,size_t* out),(feature,p,out))
FORWARD(D3D12_ReleaseFeature,(void* handle),(handle))
FORWARD(D3D12_GetFeatureRequirements,(void* adapter,const void* info,void* out),(adapter,info,out))
FORWARD(D3D12_EvaluateFeature,(void* list,const void* handle,const NVSDK_NGX_Parameter* p,void* callback),(list,handle,p,callback))
FORWARD(D3D12_PopulateParameters_Impl,(NVSDK_NGX_Parameter* p),(p))

extern "C" __declspec(dllexport) uint32_t __cdecl NVSDK_NGX_D3D12_CreateFeature(
    void* list,uint32_t feature,NVSDK_NGX_Parameter* params,void** handle) {
    using F=uint32_t(__cdecl*)(void*,uint32_t,NVSDK_NGX_Parameter*,void**);
    auto fn=Resolve<F>("NVSDK_NGX_D3D12_CreateFeature");
    if(!fn)return kFail;
    if(feature==static_cast<uint32_t>(NVSDK_NGX_Feature_FrameGeneration))
        PinLite(params);
    return fn(list,feature,params,handle);
}
