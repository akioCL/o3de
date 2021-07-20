/* Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <PrefabDependencyViewerEditorSystemComponent.h>
#include <PrefabDependencyTreeGenerator.h>

#include <Utils.h>
#include <PrefabDependencyViewerFixture.h>

namespace PrefabDependencyViewer
{
    using DirectedGraph                 = Utils::DirectedGraph;
    using TestComponent                 = PrefabDependencyViewerEditorSystemComponent;
    using Outcome                       = AZ::Outcome<PrefabDependencyTree, const char*>;

    void EXPECT_STR_EQ(const char* expected, const char* result)
    {
        EXPECT_EQ((AZStd::string) expected, (AZStd::string) result);
    }

    TEST_F(PrefabDependencyViewerFixture, INVALID_TEMPLATE_ID)
    {
        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(InvalidTemplateId,
                                                                m_prefabSystemComponent);

        EXPECT_EQ(false, outcome.IsSuccess());
    }

    TEST_F(PrefabDependencyViewerFixture, EMPTY_PREFAB_NO_SOURCE_TEST)
    {
        TemplateId tid = 10;
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(tid))
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["emptyJSON"]));

        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(10, m_prefabSystemComponent);
        EXPECT_EQ(false, outcome.IsSuccess());
        /*
        PrefabDependencyTree tree = outcome.GetValue();
        EXPECT_EQ(tid, tree.GetRoot()->GetMetaData().GetTemplateId());
        EXPECT_STR_EQ("", tree.GetRoot()->GetMetaData().GetSource());

        EXPECT_EQ(0, tree.GetChildren(tree.GetRoot()).size());
        */
    }
    
    TEST_F(PrefabDependencyViewerFixture, EMPTY_PREFAB_SAVED_TEST)
    {
        TemplateId tid = 2000;
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(tid))
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["emptySavedJSON"]));

        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(tid, m_prefabSystemComponent);
        EXPECT_EQ(true, outcome.IsSuccess());

        PrefabDependencyTree tree = outcome.GetValue();
        EXPECT_EQ(tid, tree.GetRoot()->GetMetaData().GetTemplateId());
        EXPECT_STR_EQ("Prefabs/emptySavedJSON.prefab", tree.GetRoot()->GetMetaData().GetSource());

        EXPECT_EQ(0, tree.GetChildren(tree.GetRoot()).size());
    }

    TEST_F(PrefabDependencyViewerFixture, NESTED_PREFAB_WITH_ATLEAST_ONE_INVALID_SOURCE_FILE)
    {
        // TemplateId tid = 52893;
        // Check assumptions that a prefab when not saved would have a source file. Otherwise,
        // the method FindTemplateFromSource wouldn't work and would return InvalidTemplateId.
        // EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(tid)).WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["nestedTemplateId"]));
    }

    TEST_F(PrefabDependencyViewerFixture, VALID_NESTED_PREFAB)
    {

    }
}
