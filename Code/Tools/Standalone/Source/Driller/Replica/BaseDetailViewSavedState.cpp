/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Replica/BaseDetailViewSavedState.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Driller
{
    void BaseDetailViewSplitterSavedState::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);

        if (serialize)
        {
            serialize->Class<BaseDetailViewSplitterSavedState>()
                ->Field("m_splitterStorage", &BaseDetailViewSplitterSavedState::m_splitterStorage)
                ->Version(1);
            ;
        }
    }

    void BaseDetailViewTreeSavedState::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);

        if (serialize)
        {
            serialize->Class<BaseDetailViewTreeSavedState>()
                ->Field("m_treeColumnStorage", &BaseDetailViewTreeSavedState::m_treeColumnStorage)
                ->Version(1);
        }
    }
}
