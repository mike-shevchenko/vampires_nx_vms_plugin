// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/kit/ini_config.h>
#include <nx/sdk/analytics/helpers/pixel_format.h>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace stub {
namespace settings {

struct Ini: public nx::kit::IniConfig
{
    Ini(): IniConfig("stub_analytics_plugin_settings.ini") { reload(); }

    NX_INI_FLAG(0, enableOutput, "");

    NX_INI_FLAG(0, deviceDependent, "Respective capability in the manifest.");

    NX_INI_FLAG(0, showExtraCheckBox,
        "If set, the Engine Settings Model in the Integration manifest lists an extra checkbox.");
};

Ini& ini();

} // namespace settings
} // namespace stub
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
