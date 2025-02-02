// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "device_agent.h"

#include <vector>
#include <string>
#include <mutex>
#include <chrono>
#include <ctime>
#include <thread>
#include <type_traits>

#include <nx/kit/utils.h>
#include <nx/kit/json.h>
#include <nx/sdk/analytics/helpers/event_metadata.h>
#include <nx/sdk/analytics/helpers/event_metadata_packet.h>
#include <nx/sdk/helpers/settings_response.h>

#include "../utils.h"

#include "stub_analytics_plugin_events_ini.h"

#undef NX_PRINT_PREFIX
#define NX_PRINT_PREFIX (this->logUtils.printPrefix)
#include <nx/kit/debug.h>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace stub {
namespace events {

using namespace nx::sdk;
using namespace nx::sdk::analytics;
using namespace std::chrono;

namespace {

enum class EventIsActive
{
    yes,
    no,
};

struct EventDescriptor
{
    std::string eventTypeId;
    std::string caption;
    std::string description;
    std::string prolongedEventKey;
    EventIsActive isActive;

    EventDescriptor(
        std::string eventTypeId,
        std::string caption,
        std::string description,
        EventIsActive isActive = EventIsActive::yes,
        std::string prolongedEventKey = "")
        :
        eventTypeId(std::move(eventTypeId)),
        caption(std::move(caption)),
        description(std::move(description)),
        prolongedEventKey(prolongedEventKey),
        isActive(isActive)
    {
    }
};

static const std::vector<EventDescriptor> kEventsToFire = {
    {
        kObjectInTheAreaEventType,
        "Object in the Area - prolonged event (caption) key1, STARTED",
        "Object in the Area - prolonged event (description) key1, STARTED",
        EventIsActive::yes,
        "key1"
    },
    {
        kObjectInTheAreaEventType,
        "Object in the Area - prolonged event (caption) DUPLICATE key1, STARTED",
        "Object in the Area - prolonged event (description) DUPLICATE key1, STARTED",
        EventIsActive::yes,
        "key1"
    },
    {
        kObjectInTheAreaEventType,
        "Object in the Area - prolonged event (caption) key2, STARTED",
        "Object in the Area - prolonged event (description) key2, STARTED",
        EventIsActive::yes,
        "key2"
    },
    {
        kObjectInTheAreaEventType,
        "Object in the Area - prolonged event (caption) key1, FINISHED",
        "Object in the Area - prolonged event (description) key1, FINISHED",
        EventIsActive::no,
        "key1"
    },
    {
        kObjectInTheAreaEventType,
        "Object in the Area - prolonged event (caption) NEW key1, STARTED",
        "Object in the Area - prolonged event (description) NEW key1, STARTED",
        EventIsActive::yes,
        "key1"
    },
    {
        kObjectInTheAreaEventType,
        "Object in the Area - prolonged event (caption) key2, FINISHED",
        "Object in the Area - prolonged event (description) key2, FINISHED",
        EventIsActive::no,
        "key2"
    },
    {
        kObjectInTheAreaEventType,
        "Object in the Area - prolonged event (caption) key1, FINISHED",
        "Object in the Area - prolonged event (description) key1, FINISHED",
        EventIsActive::no,
        "key1"
    },
    {
        kLineCrossingEventType,
        "Line crossing - impulse event (caption)",
        "Line crossing - impulse event (description)",
    },
    {
        kSuspiciousNoiseEventType,
        "Suspicious noise - group impulse event (caption)",
        "Suspicious noise - group impulse event (description)",
    },
    {
        kGunshotEventType,
        "Gunshot - group impulse event (caption)",
        "Gunshot - group impulse event (description)",
    },
    {
        kAdditionalEventType,
        "Caption: Additional Event",
        "Description: Additional Event",
    }
};

} // namespace

DeviceAgent::DeviceAgent(Engine* engine, const nx::sdk::IDeviceInfo* deviceInfo):
    ConsumingDeviceAgent(deviceInfo, NX_DEBUG_ENABLE_OUTPUT, engine->integration()->instanceId()),
    m_engine(engine)
{
    startEventThread();
}

DeviceAgent::~DeviceAgent()
{
    stopEventThread();
}

/**
 * DeviceAgent manifest may declare eventTypes and objectTypes similarly to how an Engine declares
 * them - semantically the set from the Engine manifest is joined with the set from the DeviceAgent
 * manifest. Also this manifest should declare the supportedTypes list which is treated as a
 * white-list filter for any type; an absent list is treated as an empty list, thus, disabling all
 * types.
 */
std::string DeviceAgent::manifestString() const
{
    static const std::string kAdditionalEventTypes = R"json(,
        {
            "id": ")json" + kAdditionalEventType + R"json(",
            "name": "Additional event 1"
        },
        {
             "id": ")json" + kAdditionalEventType2 + R"json(",
             "name": "Additional event 2"
        })json";

    std::string result = /*suppress newline*/ 1 + (const char*)
R"json(
{
    "capabilities": "disableStreamSelection",
    "supportedTypes":
    [
        { "eventTypeId": ")json" + kLineCrossingEventType + R"json(" },
        { "eventTypeId": ")json" + kSuspiciousNoiseEventType + R"json(" },
        { "eventTypeId": ")json" + kObjectInTheAreaEventType + R"json(" },
        { "eventTypeId": ")json" + kLoiteringEventType + R"json(" },
        { "eventTypeId": ")json" + kIntrusionEventType + R"json(" },
        { "eventTypeId": ")json" + kGunshotEventType + R"json(" })json"
        + (m_deviceAgentSettings.declareAdditionalEventTypes
            ? (
                R"json(,
        { "eventTypeId": ")json" + kAdditionalEventType + R"json(" },
        { "eventTypeId": ")json" + kAdditionalEventType2 + R"json(" })json"
            )
            : ""
        ) + R"json(,
        { "eventTypeId": ")json" + kEventWithImageEventType + R"json(" }
    ],
    "typeLibrary": {
        "eventTypes": [
            {
                "id": ")json" + kLoiteringEventType + R"json(",
                "name": "Loitering"
            },
            {
                "id": ")json" + kIntrusionEventType + R"json(",
                "name": "Intrusion",
                "flags": "stateDependent|regionDependent"
            },
            {
                "id": ")json" + kGunshotEventType + R"json(",
                "name": "Gunshot",
                "groupId": ")json" + kSoundRelatedEventGroup + R"json("
            })json"
            + (m_deviceAgentSettings.declareAdditionalEventTypes ? kAdditionalEventTypes : "")
            + R"json(
        ]
    }
})json";

    return result;
}

Result<const ISettingsResponse*> DeviceAgent::settingsReceived()
{
    m_deviceAgentSettings.generateEvents = toBool(settingValue(kGenerateEventsSetting));

    m_deviceAgentSettings.declareAdditionalEventTypes =
        toBool(settingValue(kDeclareAdditionalEventTypesSetting));

    // The manifest depends on declareAdditionalEventTypes setting, so sending the new manifest.
    pushManifest(manifestString());

    return nullptr;
}

void DeviceAgent::doSetNeededMetadataTypes(
    Result<void>* /*outResult*/, const IMetadataTypes* neededMetadataTypes)
{
    if (neededMetadataTypes->isEmpty())
        stopFetchingMetadata();

    startFetchingMetadata(neededMetadataTypes);
}

void DeviceAgent::startFetchingMetadata(const IMetadataTypes* /*metadataTypes*/)
{
    std::unique_lock<std::mutex> lock(m_eventThreadMutex);
    NX_OUTPUT << __func__ << "() BEGIN";
    NX_PRINT << __func__ << "(): Starting Event generation.";
    m_needToGenerateEvents = true;
    m_eventThreadCondition.notify_all();
    m_eventTypeId = kLineCrossingEventType; //< First event to produce.
    NX_OUTPUT << __func__ << "() END -> noError";
}

void DeviceAgent::stopFetchingMetadata()
{
    std::unique_lock<std::mutex> lock(m_eventThreadMutex);
    NX_OUTPUT << __func__ << "() BEGIN";
    NX_PRINT << __func__ << "(): Stopping Event generation.";
    m_needToGenerateEvents = false;
    NX_OUTPUT << __func__ << "() END -> noError";
}

void DeviceAgent::eventThreadLoop()
{
    while (!m_terminated)
    {
        if (m_deviceAgentSettings.generateEvents && m_needToGenerateEvents)
            pushMetadataPacket(cookSomeEvents());

        // Sleep until the next event needs to be generated, or the thread is ordered
        // to terminate (hence condition variable instead of sleep()). Return value
        // (whether the timeout has occurred) and spurious wake-ups are ignored.
        {
            std::unique_lock<std::mutex> lock(m_eventThreadMutex);
            if (m_terminated)
                break;
            static const milliseconds kEventGenerationPeriod{500};
            m_eventThreadCondition.wait_for(lock, kEventGenerationPeriod);
        }
    }
}

void DeviceAgent::startEventThread()
{
    m_eventThread = std::make_unique<std::thread>([this]() { eventThreadLoop(); });
}

void DeviceAgent::stopEventThread()
{
    {
        std::unique_lock<std::mutex> lock(m_eventThreadMutex);
        m_terminated = true;
        m_eventThreadCondition.notify_all();
    }

    m_eventThread->join();
}

//-------------------------------------------------------------------------------------------------
// private

Ptr<IMetadataPacket> DeviceAgent::cookSomeEvents()
{
    const auto descriptor = kEventsToFire[m_eventContext.currentEventTypeIndex];
    auto eventMetadataPacket = makePtr<EventMetadataPacket>();
    eventMetadataPacket->setTimestampUs(usSinceEpoch());
    eventMetadataPacket->setDurationUs(0);

    auto eventMetadata = makePtr<EventMetadata>();
    eventMetadata->setTypeId(descriptor.eventTypeId);
    eventMetadata->setCaption(descriptor.caption);
    eventMetadata->setDescription(descriptor.description);
    eventMetadata->setIsActive(descriptor.isActive == EventIsActive::yes);
    eventMetadata->setKey(descriptor.prolongedEventKey);

    NX_PRINT << "Generating Event: "
        << "type: " << eventMetadata->typeId()
        << ", isActive: " << eventMetadata->isActive();

    m_eventContext.currentEventTypeIndex =
        (m_eventContext.currentEventTypeIndex + 1) % ((int) kEventsToFire.size());

    eventMetadataPacket->addItem(eventMetadata);

    // Generate EventWithImage together with the first Event in the list.
    if (m_eventContext.currentEventTypeIndex == 0)
        eventMetadataPacket->addItem(createEventWithImage());

    return eventMetadataPacket;
}

Ptr<IEventMetadata> DeviceAgent::createEventWithImage()
{
    auto eventMetadata = makePtr<EventMetadata>();
    eventMetadata->setTypeId(kEventWithImageEventType);
    eventMetadata->setCaption("Event with image (caption)");
    eventMetadata->setDescription("Event with image (description)");

    const float w = 0.1F;
    const float h = 0.1F;
    const float x = m_eventContext.imageX;
    const float y = m_eventContext.imageY;
    // Intentionally allow the bounding box to spread out of the frame when close to the
    // bottom-right corner, in order to test such possibility.
    m_eventContext.imageX += 0.1F;
    if (m_eventContext.imageX >= 1.0F)
        m_eventContext.imageX = 0;
    m_eventContext.imageY += 0.1F;
    if (m_eventContext.imageY >= 1.0F)
        m_eventContext.imageY = 0;

    eventMetadata->setBoundingBox(Rect(x, y, w, h));

    NX_PRINT << "Generating EventWithImage with bounding box {" << x << ", " << y << ", "
        << w << " x " << h << "}";

    return eventMetadata;
}

int64_t DeviceAgent::usSinceEpoch() const
{
    return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}

} // namespace events
} // namespace stub
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
