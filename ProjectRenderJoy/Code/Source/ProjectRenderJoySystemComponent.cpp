
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include "ProjectRenderJoySystemComponent.h"

namespace ProjectRenderJoy
{
    void ProjectRenderJoySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ProjectRenderJoySystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ProjectRenderJoySystemComponent>("ProjectRenderJoy", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void ProjectRenderJoySystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ProjectRenderJoyService"));
    }

    void ProjectRenderJoySystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ProjectRenderJoyService"));
    }

    void ProjectRenderJoySystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void ProjectRenderJoySystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void ProjectRenderJoySystemComponent::Init()
    {
    }

    void ProjectRenderJoySystemComponent::Activate()
    {
        ProjectRenderJoyRequestBus::Handler::BusConnect();
    }

    void ProjectRenderJoySystemComponent::Deactivate()
    {
        ProjectRenderJoyRequestBus::Handler::BusDisconnect();
    }
}
