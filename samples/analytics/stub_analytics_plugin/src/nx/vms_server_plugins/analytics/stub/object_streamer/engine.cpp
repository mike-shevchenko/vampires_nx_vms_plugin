// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "engine.h"

#include "constants.h"
#include "device_agent.h"
#include "utils.h"
#include "stub_analytics_plugin_object_streamer_ini.h"

#include <nx/kit/json.h>

using nx::kit::Json;

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace stub {
namespace object_streamer {

using namespace nx::sdk;
using namespace nx::sdk::analytics;

Engine::Engine(Integration* integration):
    nx::sdk::analytics::Engine(ini().enableOutput),
    m_integration(integration)
{
}

Engine::~Engine()
{
}

void Engine::doObtainDeviceAgent(Result<IDeviceAgent*>* outResult, const IDeviceInfo* deviceInfo)
{
    *outResult = new DeviceAgent(deviceInfo, m_integration->utilityProvider()->homeDir());
}

std::string Engine::manifestString() const
{
    const std::string pluginHomeDir = m_integration->utilityProvider()->homeDir();

    Issues issues;
    const StreamInfo streamInfo = parseObjectStreamFile(
        defaultStreamFilePath(pluginHomeDir),
        &issues);

    reportIssues(issues);

    const std::string settingsModel = makeSettingsModel(
        defaultManifestFilePath(pluginHomeDir),
        defaultStreamFilePath(pluginHomeDir),
        pluginHomeDir,
        streamInfo.objectTypeIds);

    return /*suppress newline*/ 1 + (const char*)
R"json(
{
    "streamTypeFilter": "compressedVideo",
    "deviceAgentSettingsModel": )json" + settingsModel + R"json(
}
)json";
}

void Engine::reportIssues(const Issues& issues) const
{
    if (!issues.errors.empty())
    {
        pushIntegrationDiagnosticEvent(
            IIntegrationDiagnosticEvent::Level::error,
            "Serious issues in the Object stream",
            makeIntegrationDiagnosticEventDescription(issues.errors));
    }

    if (!issues.warnings.empty())
    {
        pushIntegrationDiagnosticEvent(
            IIntegrationDiagnosticEvent::Level::warning,
            "Issues in the Object stream",
            makeIntegrationDiagnosticEventDescription(issues.warnings));
    }
}

} // namespace object_streamer
} // namespace stub
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
