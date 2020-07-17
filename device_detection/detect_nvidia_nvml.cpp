#include "detect_nvidia_nvml.h"

#include "nvml.h"
#include "json.hpp"

#include <vector>
#include <windows.h>

struct NVIDIADevice {
	unsigned int index;
	std::string name;
	size_t total_memory;
};

void to_json(nlohmann::json& j, const NVIDIADevice& dev) {
	j = {
		{ "index", dev.index },
		{ "name", dev.name },
		{ "total_memory", dev.total_memory },
	};
}

bool detect_nvidia_devs(std::vector<NVIDIADevice>& out_devices);

std::string detect_nvidia_nvml::get_nvidia_nvml_devices_json_str(bool pretty_print) {
	std::vector<NVIDIADevice> devices;
	if (detect_nvidia_devs(devices)) {
		nlohmann::json j = devices;
		if (pretty_print) {
			return j.dump(4);
		}
		return j.dump();
	}
	return "[]";
}

typedef int(*nvml_Init)(void);
typedef int(*nvml_Shutdown)(void);
typedef nvmlReturn_t(*nvml_DeviceGetCount)(unsigned int* deviceCount);
typedef nvmlReturn_t(*nvml_DeviceGetHandleByIndex_v2)(unsigned int  index, nvmlDevice_t* device);
typedef nvmlReturn_t(*nvml_DeviceGetMemoryInfo)(nvmlDevice_t device, nvmlMemory_t* memory);
typedef nvmlReturn_t(*nvml_DeviceGetName)(nvmlDevice_t device, char* name, unsigned int  length);

nvml_Init NVMLInit = nullptr;
nvml_Shutdown NVMLShutdown = nullptr;
nvml_DeviceGetCount NVMLDeviceGetCount = nullptr;
nvml_DeviceGetHandleByIndex_v2 NVMLDeviceGetHandleByIndex_v2 = nullptr;
nvml_DeviceGetMemoryInfo NVMLDeviceGetMemoryInfo = nullptr;
nvml_DeviceGetName NVMLDeviceGetName = nullptr;

HMODULE load_module(const char* env_var_name, const char* sub_path) {
	char path_buffer[MAX_PATH];
	DWORD ret = GetEnvironmentVariableA(env_var_name, path_buffer, MAX_PATH);
	if (ret == 0) return 0;
	std::string path = std::string(path_buffer) + sub_path;
	return LoadLibraryA(path.c_str());
}


bool nvml_init() {
	// check standard driver install and fallback to DCH
	HMODULE hmod = load_module("ProgramFiles", "\\NVIDIA Corporation\\NVSMI\\nvml.dll");
	if (hmod == NULL) hmod = load_module("windir", "\\System32\\nvml.dll");
	if (hmod == NULL) return false;

	NVMLInit = (nvml_Init)GetProcAddress(hmod, "nvmlInit_v2");
	NVMLShutdown = (nvml_Shutdown)GetProcAddress(hmod, "nvmlShutdown");
	NVMLDeviceGetCount = (nvml_DeviceGetCount)GetProcAddress(hmod, "nvmlDeviceGetCount");
	NVMLDeviceGetHandleByIndex_v2 = (nvml_DeviceGetHandleByIndex_v2)GetProcAddress(hmod, "nvmlDeviceGetHandleByIndex_v2");
	NVMLDeviceGetMemoryInfo = (nvml_DeviceGetMemoryInfo)GetProcAddress(hmod, "nvmlDeviceGetMemoryInfo");
	NVMLDeviceGetName = (nvml_DeviceGetName)GetProcAddress(hmod, "nvmlDeviceGetName");

	int initStatus = -1;
	if (NVMLInit) {
		initStatus = NVMLInit();
	}
	return NVML_SUCCESS == initStatus;
}

void nvml_deinit() {
	if (NVMLShutdown) NVMLShutdown();
}

bool detect_nvidia_devs(std::vector<NVIDIADevice> &out_devices) {
	if (nvml_init()) {
		bool allHandles = NVMLDeviceGetCount != nullptr && NVMLDeviceGetHandleByIndex_v2 != nullptr && NVMLDeviceGetMemoryInfo != nullptr && NVMLDeviceGetName != nullptr;
		unsigned int deviceCount = 0;
		if (allHandles && NVML_SUCCESS == NVMLDeviceGetCount(&deviceCount)) {
			for (unsigned int i = 0; i < deviceCount; i++) {
				nvmlDevice_t device;
				nvmlMemory_t memory;
				char name_buff[NVML_DEVICE_NAME_BUFFER_SIZE];
				NVMLDeviceGetHandleByIndex_v2(i, &device);
				NVMLDeviceGetMemoryInfo(device, &memory);
				NVMLDeviceGetName(device, name_buff, NVML_DEVICE_NAME_BUFFER_SIZE);
				// TODO check return codes
				out_devices.push_back(NVIDIADevice{ i, std::string(name_buff), memory.total });
			}
			return true;
		}
		nvml_deinit();
	}
	return false;
}