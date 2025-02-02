// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/sdk/cloud_storage/helpers/data.h>
#include <nx/sdk/cloud_storage/i_device_agent.h>
#include <nx/sdk/helpers/ref_countable.h>
#include <nx/sdk/result.h>

namespace nx::vms_server_plugins::cloud_storage::sample {

class DeviceAgent: public nx::sdk::RefCountable<nx::sdk::cloud_storage::IDeviceAgent>
{
public:
    DeviceAgent(
        const nx::sdk::cloud_storage::DeviceDescription& deviceDescription);

protected:
    virtual void getDeviceInfo(nx::sdk::Result<const nx::sdk::IDeviceInfo*>* outResult) const override;
    virtual void doCreateStreamWriter(
        nxcip::MediaStreamQuality quality,
        int64_t startTimeMs,
        const nx::sdk::IList<nx::sdk::cloud_storage::ICodecInfo>* codecList,
        const char* opaqueMetadata,
        nx::sdk::Result<nx::sdk::cloud_storage::IStreamWriter*>* outResult) override;

    virtual void doCreateStreamReader(
        nxcip::MediaStreamQuality quality,
        int64_t startTimeMs,
        int64_t durationMs,
        nx::sdk::Result<nx::sdk::cloud_storage::IStreamReader*>* outResult) override;
};

} // namespace nx::vms_server_plugins::cloud_storage::sample
