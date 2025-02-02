// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "consuming_device_agent.h"

#include <nx/sdk/helpers/log_utils.h>
#include <nx/sdk/ptr.h>
#include <nx/sdk/helpers/string.h>
#include <nx/sdk/helpers/to_string.h>
#include <nx/sdk/helpers/error.h>
#include <nx/sdk/helpers/integration_diagnostic_event.h>
#include <nx/sdk/analytics/helpers/engine.h>

#include <nx/sdk/analytics/i_event_metadata_packet.h>
#include <nx/sdk/analytics/i_object_metadata_packet.h>
#include <nx/sdk/analytics/i_object_track_best_shot_packet.h>
#include <nx/sdk/analytics/i_object_track_title_packet.h>
#include <nx/sdk/analytics/i_integration.h>

#undef NX_PRINT_PREFIX
#define NX_PRINT_PREFIX (this->logUtils.printPrefix)
#undef NX_DEBUG_ENABLE_OUTPUT
#define NX_DEBUG_ENABLE_OUTPUT (this->logUtils.enableOutput)
#include <nx/kit/debug.h>

namespace nx::sdk::analytics {

static std::string makePrintPrefix(
    const std::string& integrationInstanceId, const IDeviceInfo* deviceInfo)
{
    const std::string& integrationInstanceIdCaption =
        integrationInstanceId.empty() ? "" : ("_" + integrationInstanceId);

    return "[" + libContext().name() + integrationInstanceIdCaption + "_device" +
        (!deviceInfo ? "" : (std::string("_") + deviceInfo->id())) + "] ";
}

ConsumingDeviceAgent::ConsumingDeviceAgent(
    const IDeviceInfo* deviceInfo, bool enableOutput, const std::string& integrationInstanceId)
    :
    logUtils(enableOutput, makePrintPrefix(integrationInstanceId, deviceInfo))
{
    NX_KIT_ASSERT(deviceInfo);
    NX_PRINT << "Created " << this;
}

ConsumingDeviceAgent::~ConsumingDeviceAgent()
{
    NX_PRINT << "Destroyed " << this;
}

//-------------------------------------------------------------------------------------------------
// Implementation of interface methods.

void ConsumingDeviceAgent::setHandler(IDeviceAgent::IHandler* handler)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handler = shareToPtr(handler);
}

void ConsumingDeviceAgent::doPushDataPacket(
    Result<void>* outResult, IDataPacket* dataPacket)
{
    const auto logError =
        [this, outResult, func = __func__](ErrorCode errorCode, const std::string& message)
        {
            NX_PRINT << func << "() " << ((NX_DEBUG_ENABLE_OUTPUT) ? "END " : "") << "-> "
                << errorCode << ": " << message;
            *outResult = error(errorCode, message);
            return;
        };

    NX_OUTPUT << __func__ << "() BEGIN";

    if (!dataPacket)
        return logError(ErrorCode::invalidParams, "dataPacket is null; discarding it.");

    if (dataPacket->timestampUs() < 0)
    {
        return logError(ErrorCode::invalidParams, "dataPacket has invalid timestamp "
            + nx::kit::utils::toString(dataPacket->timestampUs()) + "; discarding the packet.");
    }

    if (const auto compressedFrame = dataPacket->queryInterface<ICompressedVideoPacket>())
    {
        if (!pushCompressedVideoFrame(compressedFrame))
            return logError(ErrorCode::otherError, "pushCompressedVideoFrame() failed.");
    }
    else if (const auto uncompressedFrame = dataPacket->queryInterface<IUncompressedVideoFrame>())
    {
        if (!pushUncompressedVideoFrame(uncompressedFrame))
            return logError(ErrorCode::otherError, "pushUncompressedVideoFrame() failed.");
    }
    else if (const auto customMetadataPacket = dataPacket->queryInterface<ICustomMetadataPacket>())
    {
        if (!pushCustomMetadataPacket(customMetadataPacket))
            return logError(ErrorCode::otherError, "pushCustomMetadataPacket() failed.");
    }
    else
    {
        return logError(ErrorCode::invalidParams, "Unsupported frame supplied; ignored.");
    }

    if (!m_handler)
        return logError(ErrorCode::internalError, "setHandler() was not called.");

    std::vector<Ptr<IMetadataPacket>> metadataPackets;
    if (!pullMetadataPackets(&metadataPackets))
        return logError(ErrorCode::otherError, "pullMetadataPackets() failed.");

    processMetadataPackets(metadataPackets);

    NX_OUTPUT << __func__ << "() END";
}

void ConsumingDeviceAgent::processMetadataPackets(
    const std::vector<Ptr<IMetadataPacket>>& metadataPackets)
{
    if (!metadataPackets.empty())
    {
        NX_OUTPUT << __func__ << "(): Producing " << metadataPackets.size()
            << " metadata packet(s).";
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (int i = 0; i < (int) metadataPackets.size(); ++i)
            processMetadataPacket(metadataPackets.at(i), i);
    }
}

static std::string packetIndexName(int packetIndex)
{
    return packetIndex == -1 ? "" : (std::string(" #") + nx::kit::utils::toString(packetIndex));
}

void ConsumingDeviceAgent::processMetadataPacket(
    Ptr<IMetadataPacket> metadataPacket, int packetIndex /*= -1*/)
{
    if (!m_handler)
    {
        NX_PRINT << __func__ << "(): "
            << "INTERNAL ERROR: setHandler() was not called; ignoring the packet";
        return;
    }
    if (!metadataPacket)
    {
        NX_OUTPUT << __func__ << "(): WARNING: Null metadata packet"
            << packetIndexName(packetIndex) << " found; discarded.";
        return;
    }

    logMetadataPacketIfNeeded(metadataPacket, packetIndex);
    NX_KIT_ASSERT(metadataPacket->timestampUs() >= 0);
    m_handler->handleMetadata(metadataPacket.get());
}

void ConsumingDeviceAgent::getManifest(Result<const IString*>* outResult) const
{
    *outResult = new String(manifestString());
}

void ConsumingDeviceAgent::doSetSettings(
    Result<const ISettingsResponse*>* outResult, const IStringMap* settings)
{
    if (!logUtils.convertAndOutputStringMap(&m_settings, settings, "Received settings"))
        *outResult = error(ErrorCode::invalidParams, "Settings are invalid");
    else
        *outResult = settingsReceived();
}

void ConsumingDeviceAgent::finalize()
{
    NX_OUTPUT << __func__ << "()";
}

//-------------------------------------------------------------------------------------------------
// Tools for the derived class.

void ConsumingDeviceAgent::pushMetadataPacket(
    Ptr<IMetadataPacket> metadataPacket)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    processMetadataPacket(metadataPacket);
}

void ConsumingDeviceAgent::pushIntegrationDiagnosticEvent(
    IIntegrationDiagnosticEvent::Level level,
    std::string caption,
    std::string description) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_handler)
    {
        NX_PRINT << __func__ << "(): INTERNAL ERROR: "
            << "setHandler() was not called; ignoring Integration Diagnostic Event.";
        return;
    }

    const auto event = makePtr<IntegrationDiagnosticEvent>(
        level, caption, description);

    NX_OUTPUT << "Producing Integration Diagnostic Event:\n" + event->toString();

    m_handler->handleIntegrationDiagnosticEvent(event.get());
}

// TODO: Consider making a template with param type, checked according to the manifest.
std::string ConsumingDeviceAgent::settingValue(const std::string& settingName) const
{
    const auto it = m_settings.find(settingName);
    if (it != m_settings.end())
        return it->second;

    NX_PRINT << "ERROR: Requested setting "
        << nx::kit::utils::toString(settingName) << " is missing; implying empty string.";
    return "";
}

std::map<std::string, std::string> ConsumingDeviceAgent::currentSettings() const
{
    return m_settings;
}

void ConsumingDeviceAgent::pushManifest(const std::string& manifest)
{
    const auto manifestSdkString = nx::sdk::makePtr<nx::sdk::String>(manifest);
    m_handler->pushManifest(manifestSdkString.get());
}

void ConsumingDeviceAgent::logMetadataPacketIfNeeded(
    Ptr<const IMetadataPacket> metadataPacket,
    int packetIndex) const
{
    if (!NX_DEBUG_ENABLE_OUTPUT || !NX_KIT_ASSERT(metadataPacket))
        return;

    std::string packetName;
    if (metadataPacket->queryInterface<const IObjectMetadataPacket>())
    {
        packetName = "Object";
    }
    else if (metadataPacket->queryInterface<const IEventMetadataPacket>())
    {
        packetName = "Event";
    }
    else if (metadataPacket->queryInterface<const IObjectTrackBestShotPacket>())
    {
        packetName = "Best Shot";
    }
    else if (metadataPacket->queryInterface<const IObjectTrackTitlePacket>())
    {
        packetName = "Title";
    }
    else
    {
        NX_OUTPUT << __func__ << "(): WARNING: Metadata packet" << packetIndexName(packetIndex)
            << " has unknown type.";
        packetName = "Unknown";
    }
    packetName += " metadata packet" + packetIndexName(packetIndex);

    if (const auto compoundMetadataPacket =
        metadataPacket->queryInterface<const ICompoundMetadataPacket>())
    {
        if (compoundMetadataPacket->count() == 0)
        {
            NX_OUTPUT << __func__ << "(): WARNING: " << packetName << " is empty.";
            return;
        }

        const std::string itemsName = (compoundMetadataPacket->count() == 1)
            ? (std::string("item of type ") + compoundMetadataPacket->at(0)->typeId())
            : "item(s)";

        NX_OUTPUT << __func__ << "(): " << packetName << " contains "
            << compoundMetadataPacket->count() << " " << itemsName << ".";

        if (metadataPacket->timestampUs() == 0)
            NX_OUTPUT << __func__ << "(): WARNING: " << packetName << " has timestamp 0.";
    }
}

} // namespace nx::sdk::analytics
