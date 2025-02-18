// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/sdk/analytics/helpers/engine.h>
#include <nx/sdk/analytics/helpers/integration.h>

namespace nx::vms_server_plugins::analytics::stub::http_requests {

class Engine: public nx::sdk::analytics::Engine
{
public:
    Engine(nx::sdk::analytics::Integration* integration);
    virtual ~Engine() override;
    nx::sdk::Ptr<nx::sdk::IUtilityProvider> utilityProvider() const;

protected:
    virtual std::string manifestString() const override;

protected:
    virtual void doObtainDeviceAgent(
        nx::sdk::Result<nx::sdk::analytics::IDeviceAgent*>* outResult,
        const nx::sdk::IDeviceInfo* deviceInfo) override;

private:
    nx::sdk::analytics::Integration* const m_integration;
};

} // namespace nx::vms_server_plugins::analytics::stub::http_requests
