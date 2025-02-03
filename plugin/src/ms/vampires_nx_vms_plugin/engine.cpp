// Copyright 2025-present mike.shevchenko@gmail.com. Licensed under www.mozilla.org/MPL/2.0/

#include "engine.h"

#include "integration.h"
#include "device_agent.h"

namespace ms::vampires_nx_vms_plugin {

using namespace nx::sdk;
using namespace nx::sdk::analytics;

void Engine::doObtainDeviceAgent(Result<IDeviceAgent*>* outResult, const IDeviceInfo* deviceInfo)
{
    *outResult = new DeviceAgent(this, deviceInfo);
}

std::string Engine::manifestString() const
{
    return /*suppress newline*/ 1 + (const char*) R"json(
{
    "deviceAgentSettingsModel": {
        "type": "Settings",
        "items": [
            {
                "type": "GroupBox",
                "caption": "Game parameters",
                "items": [
                    {
                        "type": "SpinBox",
                        "name": ")json" + DeviceAgent::kFieldWidthSetting + R"json(",
                        "caption": "Field width",
                        "minValue": 5,
                        "defaultValue": 32
                    },
                    {
                        "type": "SpinBox",
                        "name": ")json" + DeviceAgent::kFieldHeightSetting + R"json(",
                        "caption": "Field height",
                        "minValue": 5,
                        "defaultValue": 32
                    },
                    {
                        "type": "SpinBox",
                        "name": ")json" + DeviceAgent::kVampireCountSetting + R"json(",
                        "caption": "Number of Vampires",
                        "minValue": 1,
                        "defaultValue": 8
                    },
                    {
                        "type": "SpinBox",
                        "name": ")json" + DeviceAgent::kWallCountSetting + R"json(",
                        "caption": "Number of Walls",
                        "minValue": 3,
                        "defaultValue": 100
                    },
                    {
                        "type": "SpinBox",
                        "name": ")json" + DeviceAgent::kSpeedSetting + R"json(",
                        "caption": "1/speed (frames)",
                        "minValue": 1,
                        "maxValue": 1000,
                        "defaultValue": 10
                    }
                ]
            },
            {
                "type": "GroupBox",
                "caption": "Controls",
                "items": [
                    {
                        "type": "SpinBox",
                        "name": ")json" + DeviceAgent::kPortSetting + R"json(",
                        "caption": "Socket port for control",
                        "minValue": 1,
                        "maxValue": 65535,
                        "defaultValue": 65432
                    },
                    {
                        "type": "Banner",
                        "icon": "info",
                        "text": "To connect, open a console and follow the instructions on the Server's stderr."
                    },
                    {
                        "type": "Banner",
                        "icon": "warning",
                        "text": "Don't forget to activate the Objects tab in the Client."
                    }
                ]
            }
        ]
    }
}
)json";
}

} // namespace ms::vampires_nx_vms_plugin
