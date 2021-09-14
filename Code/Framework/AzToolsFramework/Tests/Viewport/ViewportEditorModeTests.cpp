/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ViewportSelection/ViewportEditorModeStateTracker.h>

namespace UnitTest
{
    using ViewportEditorMode = AzToolsFramework::ViewportEditorMode;
    using ViewportEditorModeState = AzToolsFramework::ViewportEditorModeState;
    using ViewportEditorModeStateTracker = AzToolsFramework::ViewportEditorModeStateTracker;
    using ViewportEditorModeInfo = AzToolsFramework::ViewportEditorModeInfo;
    using ViewportId = ViewportEditorModeInfo::IdType;
    using ViewportEditorModeStateInterface = AzToolsFramework::ViewportEditorModeStateInterface;

    class EditorModeStateTestsFixture
        : public ::testing::Test
    {
    public:
        using ViewportEditorMode = AzToolsFramework::ViewportEditorMode;
        using ViewportEditorModeState = AzToolsFramework::ViewportEditorModeState;
        ViewportEditorModeState m_editorModeState;
    };

    class EditorModeStateTestsFixtureWithParams
        : public EditorModeStateTestsFixture
        , public ::testing::WithParamInterface<AzToolsFramework::ViewportEditorMode>
    {
    public:
        void SetUp() override
        {
            m_editorMode = GetParam();
        }

         void SetAllModesActive()
        {
            for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
            {
                m_editorModeState.SetModeActive(static_cast<ViewportEditorMode>(mode));
            }
        }

        void SetAllModesInactive()
        {
            for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
            {
                m_editorModeState.SetModeInactive(static_cast<ViewportEditorMode>(mode));
            }
        }

        ViewportEditorMode m_editorMode;
    };

    class ViewportEditorModeTestFixture
        : public ToolsApplicationFixture
        //, public ::testing::Test
    {
    public:
        ViewportEditorModeStateTracker m_viewportEditorMode;
    };

    class EditorModeNotificationsBusHandler
        : private AzToolsFramework::ViewportEditorModeNotificationsBus::Handler
    {
     public:
        struct ReceivedEvents
        {
            bool m_onEnter = false;
            bool m_onLeave = false;
        };

        using EditModeTracker = AZStd::unordered_map<ViewportEditorMode, ReceivedEvents>;

        EditorModeNotificationsBusHandler(ViewportId viewportId)
             : m_viewportSubscription(viewportId)
        {
            AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusConnect(m_viewportSubscription);
        }

        ~EditorModeNotificationsBusHandler()
        {
            AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
        }

        ViewportId GetViewportSubscription() const
        {
            return m_viewportSubscription;
        }

        const EditModeTracker& GetEditorModes() const
        {
            return m_editorModes;
        }

        // ViewportEditorModeNotificationsBus ...
        void OnEditorModeEnter([[maybe_unused]]const ViewportEditorModeStateInterface& editorModeState, ViewportEditorMode mode) override
        {
            m_editorModes[mode].m_onEnter = true;
        }

        virtual void OnEditorModeExit([[maybe_unused]] const ViewportEditorModeStateInterface& editorModeState, ViewportEditorMode mode) override
        {
            m_editorModes[mode].m_onLeave = true;
        }

     private:
         ViewportId m_viewportSubscription;
         EditModeTracker m_editorModes;

    };

    class ViewportEditorModePublisherTestFixture
        : public ViewportEditorModeTestFixture
    {
    public:

        void SetUpEditorFixtureImpl() override
        {
            for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
            {
                m_editorModeHandlers[mode] = AZStd::make_unique<EditorModeNotificationsBusHandler>(mode);
            }
        }

        void TearDownEditorFixtureImpl() override
        {
            for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
            {
                m_editorModeHandlers[mode].reset();
            }
        }

        AZStd::array<AZStd::unique_ptr<EditorModeNotificationsBusHandler>, ViewportEditorModeState::NumEditorModes> m_editorModeHandlers;
    };

    TEST_F(EditorModeStateTestsFixture, NumberOfEditorModesIsEqualTo4)
    {
        EXPECT_EQ(ViewportEditorModeState::NumEditorModes, 4);
    }

    TEST_F(EditorModeStateTestsFixture, InitialEditorModeStateHasAllInactiveModes)
    {
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            EXPECT_FALSE(m_editorModeState.IsModeActive(static_cast<ViewportEditorMode>(mode)));
        }
    }

    TEST_P(EditorModeStateTestsFixtureWithParams, SettingModeActiveActivatesOnlyThatMode)
    {
        m_editorModeState.SetModeActive(m_editorMode);
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            const auto editorMode = static_cast<ViewportEditorMode>(mode);
            if (editorMode == m_editorMode)
            {
                EXPECT_TRUE(m_editorModeState.IsModeActive(static_cast<ViewportEditorMode>(editorMode)));
            }
            else
            {
                EXPECT_FALSE(m_editorModeState.IsModeActive(static_cast<ViewportEditorMode>(editorMode)));
            }
        }
    }

    TEST_P(EditorModeStateTestsFixtureWithParams, SettingModeInactiveInactivatesOnlyThatMode)
    {
        SetAllModesActive();
        m_editorModeState.SetModeInactive(m_editorMode);
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            const auto editorMode = static_cast<ViewportEditorMode>(mode);
            if (editorMode == m_editorMode)
            {
                EXPECT_FALSE(m_editorModeState.IsModeActive(editorMode));
            }
            else
            {
                EXPECT_TRUE(m_editorModeState.IsModeActive(editorMode));
            }
        }
    }

    TEST_P(EditorModeStateTestsFixtureWithParams, SettingMultipleModesActiveActivatesAllThoseModesNonMutuallyExclusively)
    {
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes - 1; mode++)
        {
            SetAllModesInactive();
            m_editorModeState.SetModeActive(m_editorMode);

            const auto editorMode = static_cast<ViewportEditorMode>(mode);
            if (editorMode == m_editorMode)
            {
                continue;
            }

            m_editorModeState.SetModeActive(editorMode);
            for (auto expectedMode = 0; expectedMode < ViewportEditorModeState::NumEditorModes; expectedMode++)
            {
                const auto expectedEditorMode = static_cast<ViewportEditorMode>(expectedMode);
                if (expectedEditorMode == editorMode || expectedEditorMode == m_editorMode)
                {
                    EXPECT_TRUE(m_editorModeState.IsModeActive(expectedEditorMode));
                }
                else
                {
                    EXPECT_FALSE(m_editorModeState.IsModeActive(expectedEditorMode));
                }
            }
        }
    }

    TEST_P(EditorModeStateTestsFixtureWithParams, SettingMultipleModesInactiveInactivatesAllThoseModesNonMutuallyExclusively)
    {
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes - 1; mode++)
        {
            SetAllModesActive();
            m_editorModeState.SetModeInactive(m_editorMode);

            const auto editorMode = static_cast<ViewportEditorMode>(mode);
            if (editorMode == m_editorMode)
            {
                continue;
            }

            m_editorModeState.SetModeInactive(editorMode);
            for (auto expectedMode = 0; expectedMode < ViewportEditorModeState::NumEditorModes; expectedMode++)
            {
                const auto expectedEditorMode = static_cast<ViewportEditorMode>(expectedMode);
                if (expectedEditorMode == editorMode || expectedEditorMode == m_editorMode)
                {
                    EXPECT_FALSE(m_editorModeState.IsModeActive(expectedEditorMode));
                }
                else
                {
                    EXPECT_TRUE(m_editorModeState.IsModeActive(expectedEditorMode));
                }
            }
        }
    }

    INSTANTIATE_TEST_CASE_P(
        AllEditorModes,
        EditorModeStateTestsFixtureWithParams,
        ::testing::Values(
            AzToolsFramework::ViewportEditorMode::Default,
            AzToolsFramework::ViewportEditorMode::Component,
            AzToolsFramework::ViewportEditorMode::Focus,
            AzToolsFramework::ViewportEditorMode::Pick));

    TEST_F(EditorModeStateTestsFixture, SettingOutOfBoundsModeActiveIssuesErrorMsg)
    {
        UnitTest::TestRunner::Instance().StartAssertTests();
        m_editorModeState.SetModeActive(static_cast<ViewportEditorMode>(ViewportEditorModeState::NumEditorModes));
        EXPECT_EQ(1, UnitTest::TestRunner::Instance().StopAssertTests());
    }

    TEST_F(EditorModeStateTestsFixture, SettingOutOfBoundsModeInactiveIssuesErrorMsg)
    {
        UnitTest::TestRunner::Instance().StartAssertTests();
        m_editorModeState.SetModeInactive(static_cast<ViewportEditorMode>(ViewportEditorModeState::NumEditorModes));
        EXPECT_EQ(1, UnitTest::TestRunner::Instance().StopAssertTests());
    }

    TEST_F(ViewportEditorModeTestFixture, InitialCentralStateTrackerHasNoViewportEditorModeStates)
    {
        EXPECT_EQ(m_viewportEditorMode.GetNumTrackedViewports(), 0);
    }

    TEST_F(ViewportEditorModeTestFixture, EnteringViewportEditorModeForNonExistentIdCreatesViewportEditorModeStateForThatId)
    {
        const ViewportId viewportid = 0;
        EXPECT_FALSE(m_viewportEditorMode.IsViewportStateBeingTracked({ viewportid }));
        EXPECT_EQ(m_viewportEditorMode.GetEditorModeState({ viewportid }), nullptr);

        const auto editorMode = ViewportEditorMode::Default;
        m_viewportEditorMode.EnterMode({ viewportid }, editorMode);
        const auto* viewportEditorModeState = m_viewportEditorMode.GetEditorModeState({ viewportid });

        EXPECT_TRUE(m_viewportEditorMode.IsViewportStateBeingTracked({ viewportid }));
        EXPECT_NE(viewportEditorModeState, nullptr);
        EXPECT_TRUE(viewportEditorModeState->IsModeActive(editorMode));
    }

    TEST_F(ViewportEditorModeTestFixture, ExitingViewportEditorModeForNonExistentIdCreatesViewportEditorModeStateForThatIdButIssuesErrorMsg)
    {
        const ViewportId viewportid = 0;
        EXPECT_FALSE(m_viewportEditorMode.IsViewportStateBeingTracked({ viewportid }));
        EXPECT_EQ(m_viewportEditorMode.GetEditorModeState({ viewportid }), nullptr);

        const auto editorMode = ViewportEditorMode::Default;
        UnitTest::ErrorHandler errorHandler(AZStd::string::format(
            "Call to ExitMode for mode '%u' on id '%i' without precursor call to EnterMode", static_cast<AZ::u32>(editorMode), viewportid).c_str());
        m_viewportEditorMode.ExitMode({ viewportid }, editorMode);
        EXPECT_EQ(errorHandler.GetExpectedWarningCount(), 1);

        const auto* viewportEditorModeState = m_viewportEditorMode.GetEditorModeState({ viewportid });
        EXPECT_TRUE(m_viewportEditorMode.IsViewportStateBeingTracked({ viewportid }));
        EXPECT_NE(viewportEditorModeState, nullptr);
        EXPECT_FALSE(viewportEditorModeState->IsModeActive(editorMode));
    }

    TEST_F(ViewportEditorModeTestFixture, GettingNonExistentViewportEditorModeStateForIdReturnsNull)
    {
        const ViewportId viewportid = 0;
        EXPECT_FALSE(m_viewportEditorMode.IsViewportStateBeingTracked({ viewportid }));
        EXPECT_EQ(m_viewportEditorMode.GetEditorModeState({ viewportid }), nullptr);
    }

    TEST_F(ViewportEditorModeTestFixture, EnteringViewportEditorModeStateForExistingIdInThatStateIssuesWarningMsg)
    {
        const ViewportId viewportid = 0;
        EXPECT_FALSE(m_viewportEditorMode.IsViewportStateBeingTracked({ viewportid }));
        EXPECT_EQ(m_viewportEditorMode.GetEditorModeState({ viewportid }), nullptr);

        const auto editorMode = ViewportEditorMode::Default;
        const auto expectedWarning = AZStd::string::format(
            "Duplicate call to EnterMode for mode '%u' on id '%i'", static_cast<AZ::u32>(editorMode), viewportid);

        {
            UnitTest::ErrorHandler errorHandler(expectedWarning.c_str());
            m_viewportEditorMode.EnterMode({ viewportid }, editorMode);
            EXPECT_EQ(errorHandler.GetExpectedWarningCount(), 0);
        }
        {
            const auto* viewportEditorModeState = m_viewportEditorMode.GetEditorModeState({ viewportid });

            EXPECT_TRUE(m_viewportEditorMode.IsViewportStateBeingTracked({ viewportid }));
            EXPECT_NE(viewportEditorModeState, nullptr);
            EXPECT_TRUE(viewportEditorModeState->IsModeActive(editorMode));
        }
        {
            UnitTest::ErrorHandler errorHandler(expectedWarning.c_str());
            m_viewportEditorMode.EnterMode({ viewportid }, editorMode);
            EXPECT_EQ(errorHandler.GetExpectedWarningCount(), 1);
        }
        {
            const auto* viewportEditorModeState = m_viewportEditorMode.GetEditorModeState({ viewportid });

            EXPECT_TRUE(m_viewportEditorMode.IsViewportStateBeingTracked({ viewportid }));
            EXPECT_NE(viewportEditorModeState, nullptr);
            EXPECT_TRUE(viewportEditorModeState->IsModeActive(editorMode));
        }
    }

    TEST_F(ViewportEditorModeTestFixture, ExitingViewportEditorModeStateForExistingIdNotInThatStateIssuesWarningMsg)
    {
        const ViewportId viewportid = 0;
        EXPECT_FALSE(m_viewportEditorMode.IsViewportStateBeingTracked({ viewportid }));
        EXPECT_EQ(m_viewportEditorMode.GetEditorModeState({ viewportid }), nullptr);

        const auto editorMode = ViewportEditorMode::Default;
        const auto expectedWarning =
            AZStd::string::format("Duplicate call to ExitMode for mode '%u' on id '%i'", static_cast<AZ::u32>(editorMode), viewportid);

        {
            UnitTest::ErrorHandler errorHandler(expectedWarning.c_str());
            m_viewportEditorMode.ExitMode({ viewportid }, editorMode);
            EXPECT_EQ(errorHandler.GetExpectedWarningCount(), 0);
        }
        {
            const auto* viewportEditorModeState = m_viewportEditorMode.GetEditorModeState({ viewportid });

            EXPECT_TRUE(m_viewportEditorMode.IsViewportStateBeingTracked({ viewportid }));
            EXPECT_NE(viewportEditorModeState, nullptr);
            EXPECT_FALSE(viewportEditorModeState->IsModeActive(editorMode));
        }
        {
            UnitTest::ErrorHandler errorHandler(expectedWarning.c_str());
            m_viewportEditorMode.ExitMode({ viewportid }, editorMode);
            EXPECT_EQ(errorHandler.GetExpectedWarningCount(), 1);
        }
        {
            const auto* viewportEditorModeState = m_viewportEditorMode.GetEditorModeState({ viewportid });

            EXPECT_TRUE(m_viewportEditorMode.IsViewportStateBeingTracked({ viewportid }));
            EXPECT_NE(viewportEditorModeState, nullptr);
            EXPECT_FALSE(viewportEditorModeState->IsModeActive(editorMode));
        }
    }

    TEST_F(
        ViewportEditorModePublisherTestFixture,
        EnteringViewportEditorModeStateForExistingIdPublishesOnViewportEditorModeEnterEventForAllSubscribers)
    {
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            EXPECT_EQ(m_editorModeHandlers[mode]->GetEditorModes().size(), 0);
        }

        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            const ViewportId viewportId = mode;
            const ViewportEditorMode editorMode = static_cast<ViewportEditorMode>(mode);
            m_viewportEditorMode.EnterMode({ mode }, editorMode);
        }

        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            const ViewportEditorMode editorMode = static_cast<ViewportEditorMode>(mode);
            const auto& editorModes = m_editorModeHandlers[mode]->GetEditorModes();
            EXPECT_EQ(editorModes.size(), 1);
            EXPECT_EQ(editorModes.count(editorMode), 1);
            const auto& expectedEditorModeSet = editorModes.find(editorMode);
            EXPECT_NE(expectedEditorModeSet, editorModes.end());
            EXPECT_TRUE(expectedEditorModeSet->second.m_onEnter);
            EXPECT_FALSE(expectedEditorModeSet->second.m_onLeave);
        }
    }

    TEST_F(
        ViewportEditorModePublisherTestFixture,
        ExitingViewportEditorModeStateForExistingIdPublishesOnViewportEditorModeExitEventForAllSubscribers)
    {
        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            EXPECT_EQ(m_editorModeHandlers[mode]->GetEditorModes().size(), 0);
        }

        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            const ViewportId viewportId = mode;
            const ViewportEditorMode editorMode = static_cast<ViewportEditorMode>(mode);
            m_viewportEditorMode.EnterMode({ mode }, editorMode);
            m_viewportEditorMode.ExitMode({ mode }, editorMode);
        }

        for (auto mode = 0; mode < ViewportEditorModeState::NumEditorModes; mode++)
        {
            const ViewportEditorMode editorMode = static_cast<ViewportEditorMode>(mode);
            const auto& editorModes = m_editorModeHandlers[mode]->GetEditorModes();
            EXPECT_EQ(editorModes.size(), 1);
            EXPECT_EQ(editorModes.count(editorMode), 1);
            const auto& expectedEditorModeSet = editorModes.find(editorMode);
            EXPECT_NE(expectedEditorModeSet, editorModes.end());
            EXPECT_TRUE(expectedEditorModeSet->second.m_onEnter);
            EXPECT_TRUE(expectedEditorModeSet->second.m_onLeave);
        }
    }
}
