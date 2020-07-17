#include "device_detection.h"

#include "detect_nvidia_nvml.h"
#include "detect_open_cl.h"
#include "detect_amd_adl.h"


std::string device_detection::detect_and_get_json_str() {
    auto nvidia = detect_nvidia_nvml::get_nvidia_nvml_devices_json_str(false);
    auto open_cl = detect_open_cl::get_open_cl_devices_json_str(false);
    auto amd_adl_adapters = detect_amd_adl::get_amd_adl_devices_json_str(false);
    std::string out;
    out = R"---({"NVIDIA":)---";
    out += nvidia;
    out += R"---(,"OpenCL":)---";
    out += open_cl;
    out += R"---(,"AMD_ADL":)---";
    out += amd_adl_adapters;
    out += R"---(})---";
    return out;
}
