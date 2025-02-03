// Copyright 2025-present mike.shevchenko@gmail.com. Licensed under www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/kit/debug.h>
#include <nx/kit/utils.h>

namespace ms::vampires_nx_vms_plugin {

using SettingValueFunc = std::function<std::string(const std::string& settingName)>;

template<typename T>
concept SettingValueProvider =
    requires(T settingValueProvider, std::string settingName)
    {
        { settingValueProvider->settingValue(settingName) } -> std::convertible_to<std::string>;
    };

template<SettingValueProvider T>
int intSetting(const T& settingValueProvider, const std::string& settingName)
{
    const std::string& value = settingValueProvider->settingValue(settingName);
    try
    {
        return std::stoi(value);
    }
    catch (std::exception&)
    {
        NX_PRINT << "Invalid value in setting " << settingName << ": "
            + nx::kit::utils::toString(value);
        return -1;
    }
}

template<SettingValueProvider T>
bool boolSetting(const T& settingValueProvider, const std::string& settingName)
{
    const std::string& value = settingValueProvider->settingValue(settingName);
    if (value == "true" || value == "True" || value == "TRUE" || value == "1")
        return true;
    if (value == "false" || value == "False" || value == "FALSE" || value == "0")
        return false;

    NX_PRINT << "Invalid value in setting " << settingName << ": "
        + nx::kit::utils::toString(value);
    return false;
}

} // namespace ms::vampires_nx_vms_plugin
