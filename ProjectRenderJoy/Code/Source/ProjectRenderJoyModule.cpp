
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "ProjectRenderJoySystemComponent.h"

namespace ProjectRenderJoy
{
    class ProjectRenderJoyModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ProjectRenderJoyModule, "{b52ed5ce-bfe9-4cfd-b187-177f0d04d7c7}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ProjectRenderJoyModule, AZ::SystemAllocator, 0);

        ProjectRenderJoyModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                ProjectRenderJoySystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ProjectRenderJoySystemComponent>(),
            };
        }
    };
}// namespace ProjectRenderJoy

AZ_DECLARE_MODULE_CLASS(Project_ProjectRenderJoy, ProjectRenderJoy::ProjectRenderJoyModule)
