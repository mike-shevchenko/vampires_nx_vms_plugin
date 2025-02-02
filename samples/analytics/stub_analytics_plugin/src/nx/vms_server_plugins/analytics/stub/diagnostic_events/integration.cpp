// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "integration.h"

#include <nx/kit/debug.h>
#include <nx/kit/utils.h>

#include "engine.h"
#include "stub_analytics_integration_diagnostic_events_ini.h"

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace stub {
namespace diagnostic_events {

using namespace nx::sdk;
using namespace nx::sdk::analytics;

Result<IEngine*> Integration::doObtainEngine()
{
    return new Engine(this);
}

std::string Integration::manifestString() const
{
    return /*suppress newline*/ 1 + (const char*)
R"json(
{
    "id": ")json" + instanceId() + R"json(",
    "name": "Stub, Integration Diagnostic Events",
    "description": "A plugin for testing and debugging Integration Diagnostic Events.",
    "version": "1.0.0",
    "vendor": "Plugin vendor",
    "engineSettingsModel":
    {
        "type": "Settings",
        "items":
        [
            {
                "type": "GroupBox",
                "caption": "Stub Engine settings",
                "items":
                [
                    {
                        "type": "CheckBox",
                        "name": ")json" + kGenerateIntegrationDiagnosticEventsFromEngineSetting + R"json(",
                        "caption": "Generate Integration Diagnostic Events from the Engine",
                        "defaultValue": false
                    }
                ]
            }
        ]
    }
}
)json";
}

} // namespace diagnostic_events
} // namespace stub
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
