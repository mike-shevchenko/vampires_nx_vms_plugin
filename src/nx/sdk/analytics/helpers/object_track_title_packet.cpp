// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "object_track_title_packet.h"

#include <algorithm>

#include <nx/kit/debug.h>

namespace nx::sdk::analytics {

ObjectTrackTitlePacket::ObjectTrackTitlePacket(
    Uuid trackId,
    int64_t timestampUs,
    Rect boundingBox,
    const std::string& text)
    :
    m_trackId(trackId),
    m_timestampUs(timestampUs),
    m_boundingBox(boundingBox),
    m_text(text)
{
}

ObjectTrackTitlePacket::Flags ObjectTrackTitlePacket::flags() const
{
    return m_flags;
}

int64_t ObjectTrackTitlePacket::timestampUs() const
{
    return m_timestampUs;
}

void ObjectTrackTitlePacket::getBoundingBox(Rect* outValue) const
{
    *outValue = m_boundingBox;
}

void ObjectTrackTitlePacket::getTrackId(Uuid* outValue) const
{
    *outValue = m_trackId;
}

const char* ObjectTrackTitlePacket::text() const
{
    return m_text.c_str();
}

const char* ObjectTrackTitlePacket::imageUrl() const
{
    return m_imageUrl.c_str();
}

const char* ObjectTrackTitlePacket::imageData() const
{
    return m_imageData.data();
}

int ObjectTrackTitlePacket::imageDataSize() const
{
    return (int) m_imageData.size();
}

const char* ObjectTrackTitlePacket::imageDataFormat() const
{
    return m_imageDataFormat.c_str();
}

void ObjectTrackTitlePacket::setFlags(Flags flags)
{
    m_flags = flags;
}

void ObjectTrackTitlePacket::setTimestampUs(int64_t timestampUs)
{
    m_timestampUs = timestampUs;
}

void ObjectTrackTitlePacket::setBoundingBox(const Rect& boundingBox)
{
    m_boundingBox = boundingBox;
}

void ObjectTrackTitlePacket::setTrackId(const Uuid& trackId)
{
    m_trackId = trackId;
}

void ObjectTrackTitlePacket::setText(std::string text)
{
    m_text = std::move(text);
}

void ObjectTrackTitlePacket::setImageUrl(std::string imageUrl)
{
    m_imageUrl = std::move(imageUrl);
}

void ObjectTrackTitlePacket::setImageDataFormat(std::string imageDataFormat)
{
    m_imageDataFormat = std::move(imageDataFormat);
}

void ObjectTrackTitlePacket::setImageData(std::vector<char> imageData)
{
    m_imageData = std::move(imageData);
}

void ObjectTrackTitlePacket::setImage(std::string imageDataFormat, std::vector<char> imageData)
{
    setImageDataFormat(std::move(imageDataFormat));
    setImageData(std::move(imageData));
}

} // namespace nx::sdk::analytics
