#include "detect_amd_adl.h"

#include "json.hpp"

// TODO add PCI bus and total memory
bool get_amd_adl_adapters(std::vector<std::string>& out_adapter_names);

std::string detect_amd_adl::get_amd_adl_devices_json_str(bool pretty_print) {
    std::vector<std::string> adapter_names;
    if (get_amd_adl_adapters(adapter_names)) {
        nlohmann::json j = adapter_names;
        if (pretty_print) {
            return j.dump(4);
        }
        return j.dump();
    }
    return "[]";
}

void detect_amd_adl::fill_amd_device_names(std::vector<std::string>& devices) {
    get_amd_adl_adapters(devices);
}

#include <windows.h>
#include "adl_sdk.h"
#include "adl_structures.h"

#include <stdio.h>
#include <set>
#include <memory>
#include <optional>
#include <math.h>

// Comment out one of the two lines below to allow or supress diagnostic messages
// #define PRINTF
#define PRINTF printf

// Definitions of the used function pointers. Add more if you use other ADL APIs
typedef int(*ADL_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int);
typedef int(*ADL_MAIN_CONTROL_DESTROY)();
//typedef int(*ADL_FLUSH_DRIVER_DATA)(int);
typedef int(*ADL2_ADAPTER_ACTIVE_GET) (ADL_CONTEXT_HANDLE, int, int*);

typedef int(*ADL_ADAPTER_NUMBEROFADAPTERS_GET) (int*);
typedef int(*ADL_ADAPTER_ADAPTERINFO_GET) (LPAdapterInfo, int);
typedef int(*ADL2_ADAPTER_MEMORYINFO_GET)(ADL_CONTEXT_HANDLE, int, ADLMemoryInfo*);


HINSTANCE hDLL;

ADL_MAIN_CONTROL_CREATE          ADL_Main_Control_Create = NULL;
ADL_MAIN_CONTROL_DESTROY         ADL_Main_Control_Destroy = NULL;
ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get = NULL;
ADL_ADAPTER_ADAPTERINFO_GET      ADL_Adapter_AdapterInfo_Get = NULL;
ADL2_ADAPTER_MEMORYINFO_GET      ADL2_Adapter_MemoryInfo_Get = NULL;

// Memory allocation function
void* __stdcall ADL_Main_Memory_Alloc(int iSize)
{
    void* lpBuffer = malloc(iSize);
    return lpBuffer;
}

// Optional Memory de-allocation function
void __stdcall ADL_Main_Memory_Free(void** lpBuffer)
{
    if (NULL != *lpBuffer)
    {
        free(*lpBuffer);
        *lpBuffer = NULL;
    }
}

ADL_CONTEXT_HANDLE context = NULL;

LPAdapterInfo   lpAdapterInfo = NULL;
int  iNumberAdapters;

int initializeADL()
{

    // Load the ADL dll
    hDLL = LoadLibraryA("atiadlxx.dll");
    if (hDLL == NULL)
    {
        // A 32 bit calling application on 64 bit OS will fail to LoadLibrary.
        // Try to load the 32 bit library (atiadlxy.dll) instead
        hDLL = LoadLibraryA("atiadlxy.dll");
    }

    if (NULL == hDLL)
    {
        PRINTF("Failed to load ADL library\n");
        return FALSE;
    }
    ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE)GetProcAddress(hDLL, "ADL_Main_Control_Create");
    ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY)GetProcAddress(hDLL, "ADL_Main_Control_Destroy");
    ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET)GetProcAddress(hDLL, "ADL_Adapter_NumberOfAdapters_Get");
    ADL_Adapter_AdapterInfo_Get = (ADL_ADAPTER_ADAPTERINFO_GET)GetProcAddress(hDLL, "ADL_Adapter_AdapterInfo_Get");
    ADL2_Adapter_MemoryInfo_Get = (ADL2_ADAPTER_MEMORYINFO_GET)GetProcAddress(hDLL, "ADL2_Adapter_MemoryInfo_Get");

    if (NULL == ADL_Main_Control_Create ||
        NULL == ADL_Main_Control_Destroy ||
        NULL == ADL_Adapter_NumberOfAdapters_Get ||
        NULL == ADL_Adapter_AdapterInfo_Get
        )
    {
        PRINTF("Failed to get ADL function pointers\n");
        return FALSE;
    }

    if (ADL_OK != ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 1))
    {
        printf("Failed to initialize nested ADL2 context");
        return ADL_ERR;
    }

    return TRUE;
}

void deinitializeADL()
{
    ADL_Main_Control_Destroy();

    FreeLibrary(hDLL);
}

std::optional<std::string> convert_size2(double size) {
    static const std::array units = { "B", "KB", "MB", "GB", "TB", "PB" };
    static const double mod = 1024.0;
    int i = 0;
    while (size >= mod && i < units.size()) {
        size /= mod;
        i++;
    }
    if (units.size()) return std::to_string((int)(ceil(size))) + units[i];
    return std::nullopt;
}

bool get_amd_adl_adapters(std::vector<std::string> &out_adapter_names) {
    if (initializeADL())
    {

        // Obtain the number of adapters for the system
        if (ADL_OK != ADL_Adapter_NumberOfAdapters_Get(&iNumberAdapters))
        {
            PRINTF("Cannot get the number of adapters!\n");
            return false;
        }

        if (0 < iNumberAdapters)
        {
            lpAdapterInfo = (LPAdapterInfo)malloc(sizeof(AdapterInfo) * iNumberAdapters);
            memset(lpAdapterInfo, '\0', sizeof(AdapterInfo) * iNumberAdapters);
            // Get the AdapterInfo structure for all adapters in the system
            ADL_Adapter_AdapterInfo_Get(lpAdapterInfo, sizeof(AdapterInfo) * iNumberAdapters);
        }
        std::set<int> added_bus_numbers;
        for (int i = 0; i < iNumberAdapters; i++) {
            // AMD only 
            if (lpAdapterInfo[i].iVendorID != 1002) continue;
            if (added_bus_numbers.find(lpAdapterInfo[i].iBusNumber) == added_bus_numbers.end()) {
                added_bus_numbers.insert(lpAdapterInfo[i].iBusNumber);
                ADLMemoryInfo lpMemoryInfo;
                //ADLMemoryInfo lpMemoryInfo = (ADLMemoryInfo)malloc(sizeof(ADLMemoryInfo));
                if (ADL_OK == ADL2_Adapter_MemoryInfo_Get(context, lpAdapterInfo[i].iAdapterIndex, &lpMemoryInfo)) {
                    if (auto postfix = convert_size2(lpMemoryInfo.iMemorySize); postfix.has_value()) {
                        out_adapter_names.push_back(std::string(lpAdapterInfo[i].strAdapterName) + " " + postfix.value());
                        continue;
                    }
                }
                // no memory
                out_adapter_names.push_back(std::string(lpAdapterInfo[i].strAdapterName));
            }
        }
        ADL_Main_Memory_Free((void**)&lpAdapterInfo);
        deinitializeADL();
    }
}