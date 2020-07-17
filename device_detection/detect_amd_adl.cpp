#include "detect_amd_adl.h"

#include "json.hpp"
#include <vector>

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

#include <windows.h>
#include "adl_sdk.h"
#include "adl_structures.h"

#include <stdio.h>
#include <set>
#include <memory>

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


HINSTANCE hDLL;

ADL_MAIN_CONTROL_CREATE          ADL_Main_Control_Create = NULL;
ADL_MAIN_CONTROL_DESTROY         ADL_Main_Control_Destroy = NULL;
ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get = NULL;
ADL_ADAPTER_ADAPTERINFO_GET      ADL_Adapter_AdapterInfo_Get = NULL;

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
    hDLL = LoadLibrary(TEXT("atiadlxx.dll"));
    if (hDLL == NULL)
    {
        // A 32 bit calling application on 64 bit OS will fail to LoadLibrary.
        // Try to load the 32 bit library (atiadlxy.dll) instead
        hDLL = LoadLibrary(TEXT("atiadlxy.dll"));
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
                out_adapter_names.push_back(std::string(lpAdapterInfo[i].strAdapterName));
            }
        }
        ADL_Main_Memory_Free((void**)&lpAdapterInfo);
        deinitializeADL();
    }
}