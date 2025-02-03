// Copyright 2025-present mike.shevchenko@gmail.com. Licensed under www.mozilla.org/MPL/2.0/

#include "device_agent.h"

#include <nx/sdk/analytics/helpers/event_metadata.h>
#include <nx/sdk/analytics/helpers/event_metadata_packet.h>
#include <nx/sdk/analytics/helpers/object_metadata_packet.h>

#include "integration.h"
#include "utils.h"

namespace ms::vampires_nx_vms_plugin {

using namespace nx::sdk;
using namespace nx::sdk::analytics;

/** Adds a constant uuid to the item. */
class Item: public Vampires::Item
{
public:
    Item(Kind kind, int x, int y):
        Vampires::Item(kind, x, y),
        uuid(nx::sdk::UuidHelper::randomUuid())
    {
    }

    virtual std::string toString() const override
    {
        return Vampires::Item::toString() + nx::sdk::UuidHelper::toStdString(uuid);
    }

public:
    const nx::sdk::Uuid uuid;
};

class ItemFactory: public Vampires::Item::Factory
{
public:
    virtual Item* createItem(Vampires::Item::Kind kind, int x, int y) const override
    {
        return new Item(kind, x, y);
    }
};

std::string DeviceAgent::manifestString() const
{
    return /*suppress newline*/ 1 + (const char*) R"json(
{
    "typeLibrary": {
        "objectTypes": [
            {
                "id": ")json" + kPlayerObjectType + R"json(",
                "name": ""
            },
            {
                "id": ")json" + kWallObjectType + R"json(",
                "name": ""
            },
            {
                "id": ")json" + kVampireObjectType + R"json(",
                "name": ""
            },
            {
                "id": ")json" + kBorderObjectType + R"json(",
                "name": ""
            }
        ]
    }
}
)json";
}

static std::string itemColor(Vampires::Item::Kind kind)
{
    switch (kind)
    {
        case Vampires::Item::Kind::player: return "Cyan";
        case Vampires::Item::Kind::wall: return "Green";
        case Vampires::Item::Kind::vampire: return "Magenta";
        case Vampires::Item::Kind::border: return "Red";
        default:
            NX_KIT_ASSERT(false);
            return "Orange";
    }
}

/** If the key is irrelevant, return Direction::count. */
static Vampires::Direction keyToDirection(int key)
{
    switch (key)
    {
        // We need diagonal keys, so we use keypad in Numlock mode, or qwe/asd/zx.
        case '7': case 'q': return Vampires::Direction::upLeft;
        case '8': case 'w': return Vampires::Direction::up;
        case '9': case 'e': return Vampires::Direction::upRight;
        case '4': case 'a': return Vampires::Direction::left;
        case '5': case 's': case '2': return Vampires::Direction::down;
        case '6': case 'd': return Vampires::Direction::right;
        case '1': case 'z': return Vampires::Direction::downLeft;
        case '3': case 'x': return Vampires::Direction::downRight;
        default: return Vampires::Direction::count;
    }
}

void DeviceAgent::performPlayerLost()
{
    pushIntegrationDiagnosticEvent(
        IIntegrationDiagnosticEvent::Level::error, "Game over", "Vampire has you!");
    initGame();
}

void DeviceAgent::performPlayerWon()
{
    pushIntegrationDiagnosticEvent(
        IIntegrationDiagnosticEvent::Level::warning, "Congratulations", "Vampires pwned!");
    initGame();
}

/**
 * Called when the Server sends a new uncompressed frame from a camera.
 */
bool DeviceAgent::pushCompressedVideoFrame(Ptr<const ICompressedVideoPacket> videoFrame)
{
    ++m_frameIndex;
    m_lastVideoFrameTimestampUs = videoFrame->timestampUs();

    if (!NX_KIT_ASSERT(m_socketReader))
        return false;

    if (const std::optional<char> key = m_socketReader->getChar())
    {
        const Vampires::Direction direction = keyToDirection(*key);
        if (direction != Vampires::Direction::count)
        {
            if (m_vampires->movePlayer(direction) == Vampires::PlayerResult::lost)
                performPlayerLost();
        }

        m_socketReader->clear(); //< Needed if keyboard repeat rate is more than the frame rate.
    }

    // Move the vampires every Nth frame.
    if (m_frameIndex % intSetting(this, kSpeedSetting) == 0)
    {
        switch (m_vampires->moveVampires())
        {
            case Vampires::VampireResult::lost:
                performPlayerLost();
                break;
            case Vampires::VampireResult::win:
                performPlayerWon();
                break;
        }
    }

    return true; //< There were no errors while processing the video frame.
}

bool DeviceAgent::pullMetadataPackets(std::vector<Ptr<IMetadataPacket>>* metadataPackets)
{
    metadataPackets->push_back(generateObjectMetadataPacket());
    return true; //< There were no errors while filling metadataPackets.
}

void DeviceAgent::initGame()
{
    m_vampires = std::make_unique<Vampires>(
        intSetting(this, kFieldWidthSetting),
        intSetting(this, kFieldHeightSetting),
        intSetting(this, kVampireCountSetting),
        intSetting(this, kWallCountSetting),
        std::make_shared<ItemFactory>());
}

void DeviceAgent::doSetNeededMetadataTypes(
    nx::sdk::Result<void>* /*outValue*/,
    const nx::sdk::analytics::IMetadataTypes* /*neededMetadataTypes*/)
{
    m_socketReader = std::make_unique<SocketReader>();

    if (!m_socketReader->startListening(intSetting(this, kPortSetting)))
    {
        NX_PRINT << "FATAL ERROR: Unable to open the control socket. Terminating.";
        exit(42);
    }

    NX_PRINT << "Control keys: keypad with NumLock, or qwe/asd/zx - make use of diagonal keys!";

    initGame();
}

//-------------------------------------------------------------------------------------------------
// private

std::string DeviceAgent::itemObjectType(Vampires::Item::Kind kind) const
{
    switch (kind)
    {
        case Vampires::Item::Kind::player: return kPlayerObjectType;
        case Vampires::Item::Kind::wall: return kWallObjectType;
        case Vampires::Item::Kind::vampire: return kVampireObjectType;
        case Vampires::Item::Kind::border: return kBorderObjectType;
        default:
            NX_KIT_ASSERT(false);
            return "<unknown>";
    }
}

Ptr<ObjectMetadata> DeviceAgent::createObjectMetadata(const Item* item) const
{
    if (!item)
        return nullptr;

    auto objectMetadata = makePtr<ObjectMetadata>();
    objectMetadata->setTypeId(itemObjectType(item->kind));
    objectMetadata->setTrackId(item->uuid);
    const float cellWidth = 1.0F / m_vampires->width;
    const float cellHeight = 1.0F / m_vampires->height;
    objectMetadata->setBoundingBox(Rect(
        (float) item->x() * cellWidth, (float) item->y() * cellHeight, cellWidth, cellHeight));

    std::vector<Ptr<Attribute>> attributes;
    attributes.push_back(makePtr<Attribute>(
        Attribute::Type::string, "nx.sys.color", itemColor(item->kind)));
    objectMetadata->addAttributes(attributes);
    return objectMetadata;
}

Ptr<IMetadataPacket> DeviceAgent::generateObjectMetadataPacket() const
{
    // ObjectMetadataPacket contains arbitrary number of ObjectMetadata.
    const auto objectMetadataPacket = makePtr<ObjectMetadataPacket>();

    // Bind the object metadata to the last video frame using a timestamp.
    objectMetadataPacket->setTimestampUs(m_lastVideoFrameTimestampUs);
    objectMetadataPacket->setDurationUs(0);

    for (int y = 0; y < m_vampires->height; ++y)
    {
        for (int x = 0; x < m_vampires->width; ++x)
        {
            if (const auto objectMetadata = createObjectMetadata(
                dynamic_cast<const Item*>(m_vampires->itemAt(x, y).get())))
            {
                objectMetadataPacket->addItem(objectMetadata);
            }
        }
    }

    return objectMetadataPacket;
}

} // namespace ms::vampires_nx_vms_plugin
