// Copyright 2025-present mike.shevchenko@gmail.com. Licensed under www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/sdk/analytics/helpers/integration.h>
#include <nx/sdk/analytics/helpers/engine.h>
#include <nx/sdk/analytics/i_uncompressed_video_frame.h>

namespace ms::vampires_nx_vms_plugin {

class Integration;

class Engine: public nx::sdk::analytics::Engine
{
public:
    Engine(Integration* integration):
        nx::sdk::analytics::Engine(/*enableOutput*/ false),
        m_integration(integration)
    {
    }

    virtual ~Engine() override = default;

    std::string settingValue(const std::string& settingName) const
    {
        return nx::sdk::analytics::Engine::settingValue(settingName);
    }

    Integration* integration() const { return m_integration; }

protected:
    virtual std::string manifestString() const override;

    virtual void doObtainDeviceAgent(
        nx::sdk::Result<nx::sdk::analytics::IDeviceAgent*>* outResult,
        const nx::sdk::IDeviceInfo* deviceInfo) override;

private:
    Integration* const m_integration;
};

} // namespace ms::vampires_nx_vms_plugin
