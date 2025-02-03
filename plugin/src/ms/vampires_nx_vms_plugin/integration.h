// Copyright 2025-present mike.shevchenko@gmail.com. Licensed under www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/sdk/analytics/helpers/integration.h>
#include <nx/sdk/analytics/i_engine.h>

namespace ms::vampires_nx_vms_plugin {

class Integration: public nx::sdk::analytics::Integration
{
protected:
    virtual nx::sdk::Result<nx::sdk::analytics::IEngine*> doObtainEngine() override;
    virtual std::string manifestString() const override;
};

} // namespace ms::vampires_nx_vms_plugin
