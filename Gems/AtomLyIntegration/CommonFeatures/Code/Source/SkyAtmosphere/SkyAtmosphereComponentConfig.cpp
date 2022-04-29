/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/SkyAtmosphere/SkyAtmosphereComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::Render
{
    void SkyAtmosphereComponentConfig::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SkyAtmosphereComponentConfig, ComponentConfig>()
                ->Version(1)
                ->Field("AtmosphereRadius", &SkyAtmosphereComponentConfig::m_atmosphereRadius)
                ->Field("PlanetRadius", &SkyAtmosphereComponentConfig::m_planetRadius)
                ->Field("DrawSun", &SkyAtmosphereComponentConfig::m_drawSun)
                ->Field("SunOrientation", &SkyAtmosphereComponentConfig::m_sun)
                ->Field("SunRadiusFactor", &SkyAtmosphereComponentConfig::m_sunRadiusFactor)
                ->Field("SunFalloffFactor", &SkyAtmosphereComponentConfig::m_sunFalloffFactor)
                ->Field("SunIlluminance", &SkyAtmosphereComponentConfig::m_sunIlluminance)
                ->Field("MinSamples", &SkyAtmosphereComponentConfig::m_minSamples)
                ->Field("MaxSamples", &SkyAtmosphereComponentConfig::m_maxSamples)
                ->Field("GroundAlbedo", &SkyAtmosphereComponentConfig::m_groundAlbedo)
                ->Field("MieExtinction", &SkyAtmosphereComponentConfig::m_mieExtinction)
                ->Field("MieScattering", &SkyAtmosphereComponentConfig::m_mieScattering)
                ->Field("AbsorptionExtinction", &SkyAtmosphereComponentConfig::m_absorptionExtinction)
                ->Field("RaleighScattering", &SkyAtmosphereComponentConfig::m_rayleighScattering)
                ->Field("OriginMode", &SkyAtmosphereComponentConfig::m_originMode)
                ->Field("FastSkyEnabled", &SkyAtmosphereComponentConfig::m_fastSkyEnabled)
                ->Field("FastSkyLUTSize", &SkyAtmosphereComponentConfig::m_fastSkyLUTSize)
                ;
        }
    }
} // AZ::Render
