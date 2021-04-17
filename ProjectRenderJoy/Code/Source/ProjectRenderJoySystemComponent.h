
#pragma once

#include <AzCore/Component/Component.h>

#include <ProjectRenderJoy/ProjectRenderJoyBus.h>

namespace ProjectRenderJoy
{
    class ProjectRenderJoySystemComponent
        : public AZ::Component
        , protected ProjectRenderJoyRequestBus::Handler
    {
    public:
        AZ_COMPONENT(ProjectRenderJoySystemComponent, "{760d5707-128e-45c7-88ce-d49f68d7f039}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // ProjectRenderJoyRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
