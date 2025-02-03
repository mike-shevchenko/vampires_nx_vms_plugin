// Copyright 2025-present mike.shevchenko@gmail.com. Licensed under www.mozilla.org/MPL/2.0/

#pragma once

#include <memory>

#include <nx/sdk/analytics/helpers/consuming_device_agent.h>
#include <nx/sdk/helpers/uuid_helper.h>
#include <nx/sdk/analytics/helpers/object_metadata.h>

#include "engine.h"
#include "socket_reader.h"
#include "vampires.h"

namespace ms::vampires_nx_vms_plugin {

class Item;

class DeviceAgent: public nx::sdk::analytics::ConsumingDeviceAgent
{
public:
    DeviceAgent(Engine* const engine, const nx::sdk::IDeviceInfo* deviceInfo):
        ConsumingDeviceAgent(deviceInfo, /*enableOutput*/ false),
        m_engine(engine)
    {
    }

    virtual ~DeviceAgent() override = default;

    std::string settingValue(const std::string& settingName) const
    {
        return nx::sdk::analytics::ConsumingDeviceAgent::settingValue(settingName);
    }

    static inline const std::string kFieldWidthSetting = "fieldWidth";
    static inline const std::string kFieldHeightSetting = "fieldHeight";
    static inline const std::string kVampireCountSetting = "vampireCount";
    static inline const std::string kWallCountSetting = "wallCount";
    static inline const std::string kSpeedSetting = "speed";
    static inline const std::string kPortSetting = "port";

protected:
    virtual std::string manifestString() const override;

    virtual bool pushCompressedVideoFrame(
        nx::sdk::Ptr<const nx::sdk::analytics::ICompressedVideoPacket> videoFrame) override;

    virtual bool pullMetadataPackets(
        std::vector<nx::sdk::Ptr<nx::sdk::analytics::IMetadataPacket>>* metadataPackets) override;

    virtual void doSetNeededMetadataTypes(
        nx::sdk::Result<void>* outValue,
        const nx::sdk::analytics::IMetadataTypes* neededMetadataTypes) override;

private:
    nx::sdk::Ptr<nx::sdk::analytics::IMetadataPacket> generateObjectMetadataPacket() const;
    std::string itemObjectType(Vampires::Item::Kind kind) const;

    nx::sdk::Ptr<nx::sdk::analytics::ObjectMetadata> createObjectMetadata(const Item* item) const;

    void performPlayerLost();
    void performPlayerWon();
    void initGame();

private:
    static inline const std::string kPlayerObjectType = "ms.vampires.player";
    static inline const std::string kWallObjectType = "ms.vampires.wall";
    static inline const std::string kVampireObjectType = "ms.vampires.vampire";
    static inline const std::string kBorderObjectType = "ms.vampires.border";

    Engine* const m_engine;

    /** Length of the the track (in frames). The value was chosen arbitrarily. */
    static constexpr int kTrackFrameCount = 256;

    nx::sdk::Uuid m_trackId = nx::sdk::UuidHelper::randomUuid();
    int m_frameIndex = 0; /**< Used for generating the detection in the right place. */
    int m_trackIndex = 0; /**< Used in the description of the events. */

    /** Used for binding object and event metadata to the particular video frame. */
    int64_t m_lastVideoFrameTimestampUs = 0;

    std::unique_ptr<Vampires> m_vampires;
    std::unique_ptr<SocketReader> m_socketReader;
};

} // namespace ms::vampires_nx_vms_plugin
