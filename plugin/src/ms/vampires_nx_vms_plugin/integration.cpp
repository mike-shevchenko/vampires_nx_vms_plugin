// Copyright 2025-present mike.shevchenko@gmail.com. Licensed under www.mozilla.org/MPL/2.0/

#include "integration.h"

#include "engine.h"

#include <nx/kit/utils.h>

namespace ms::vampires_nx_vms_plugin {

using namespace nx::sdk;
using namespace nx::sdk::analytics;

Result<IEngine*> Integration::doObtainEngine()
{
    return new Engine(this);
}

/**
 * JSON with the particular structure. Note that it is possible to fill in the values that are not
 * known at compile time.
 *
 * - id: Unique identifier for an Integration with format "{vendor_id}.{integration_id}", where
 *     {vendor_id} is the unique identifier of the integration creator (person or company name) and
 *     {integration_id} is the unique (for a specific vendor) identifier of the Integration.
 * - name: A human-readable short name of the Integration (displayed in the "Camera Settings"
 *     window of the Client).
 * - description: Description of the Integration in a few sentences.
 * - version: Version of the Integration.
 * - vendor: Integration creator (person or company) name.
 */
std::string Integration::manifestString() const
{
    return /*suppress newline*/ 1 + (const char*) R"json(
{
    "id": "ms.vampires",
    "name": "Vampires Plugin",
    "description": "Game from the Soviet computer AGAT by Roman Bader, 1987.",
    "version": "3.0.0",
    "vendor": "Mike Shevchenko (mike.shevchenko@gmail.com)"
}
)json";
}

/**
 * Called by the Server to instantiate the Integration object.
 *
 * The Server requires the function to have C linkage, which leads to no C++ name mangling in the
 * export table of the plugin dynamic library, so that makes it possible to write plugins in any
 * language and compiler.
 *
 * NX_PLUGIN_API is the macro defined by CMake scripts for exporting the function.
 */
extern "C" NX_PLUGIN_API nx::sdk::IIntegration* createNxPlugin()
{
    // The object will be freed when the Server calls releaseRef().
    return new Integration();
}

} // namespace ms::vampires_nx_vms_plugin
