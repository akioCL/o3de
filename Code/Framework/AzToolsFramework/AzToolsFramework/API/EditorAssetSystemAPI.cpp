/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace AzToolsFramework
{
    void AssetSystem::JobInfo::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<JobInfo>()
                ->Version(4)
                ->Field("sourceFile", &JobInfo::m_sourceFile)
                ->Field("platform", &JobInfo::m_platform)
                ->Field("builderUuid", &JobInfo::m_builderGuid)
                ->Field("jobKey", &JobInfo::m_jobKey)
                ->Field("jobRunKey", &JobInfo::m_jobRunKey)
                ->Field("status", &JobInfo::m_status)
                ->Field("firstFailLogTime", &JobInfo::m_firstFailLogTime)
                ->Field("firstFailLogFile", &JobInfo::m_firstFailLogFile)
                ->Field("lastFailLogTime", &JobInfo::m_lastFailLogTime)
                ->Field("lastFailLogFile", &JobInfo::m_lastFailLogFile)
                ->Field("lastLogTime", &JobInfo::m_lastLogTime)
                ->Field("lastLogFile", &JobInfo::m_lastLogFile)
                ->Field("jobID", &JobInfo::m_jobID)
                ->Field("watchFolder", &JobInfo::m_watchFolder)
                ->Field("errorCount", &JobInfo::m_errorCount)
                ->Field("warningCount", &JobInfo::m_warningCount)
                ;
        }
    }
}
            