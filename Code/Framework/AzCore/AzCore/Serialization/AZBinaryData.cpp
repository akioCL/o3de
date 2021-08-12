/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/AZBinaryData.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Memory/Memory.h>

namespace AZ::Internal
{
    size_t AZBinaryData::DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/)
    {
        (void)isDataBigEndian;
        const size_t dataSize = static_cast<size_t>(in.GetLength()); // each byte is encoded in 2 chars (hex)

        AZStd::string outStr;
        if (dataSize)
        {
            AZ::u8* buffer = static_cast<AZ::u8*>(azmalloc(dataSize));

            in.Read(dataSize, reinterpret_cast<void*>(buffer));

            const AZ::u8* first = buffer;
            const AZ::u8* last = first + dataSize;
            if (first == last)
            {
                azfree(buffer);
                outStr.clear();
                return 1; // for no data (just terminate)
            }

            outStr.resize(dataSize * 2);

            char* data = outStr.data();
            for (; first != last; ++first, data += 2)
            {
                azsnprintf(data, 3, "%02X", *first);
            }

            azfree(buffer);
        }

        return static_cast<size_t>(out.Write(outStr.size(), outStr.data()));
    }

    size_t AZBinaryData::TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian /*= false*/)
    {
        (void)textVersion;
        (void)isDataBigEndian;
        size_t textLenth = strlen(text);
        size_t minDataSize = textLenth / 2;

        const char* textEnd = text + textLenth;
        char workBuffer[3];
        workBuffer[2] = '\0';
        for (; text != textEnd; text += 2)
        {
            workBuffer[0] = text[0];
            workBuffer[1] = text[1];
            AZ::u8 value = static_cast<AZ::u8>(strtoul(workBuffer, nullptr, 16));
            stream.Write(sizeof(AZ::u8), &value);
        }
        return minDataSize;
    }
}
