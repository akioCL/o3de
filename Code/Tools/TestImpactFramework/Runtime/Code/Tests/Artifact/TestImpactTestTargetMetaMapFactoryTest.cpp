/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <TestImpactTestUtils.h>

#include <Artifact/Factory/TestImpactTestTargetMetaMapFactory.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    TEST(TestTargetMetaMapFactoryTest, NoRawData_ExpectArtifactException)
    {
        // Given an empty meta data string
        const AZStd::string rawTestTargetMetaData;

        try
        {
            // When attempting to construct the test target
            const TestImpact::TestTargetMetaMap testTargetMetaData = TestImpact::TestTargetMetaMapFactory(rawTestTargetMetaData);

            // Do not expect this statement to be reachable
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::ArtifactException& e)
        {
            // Expect an artifact exception
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(TestTargetMetaMapFactoryTest, InvalidRawData_ExpectArtifactException)
    {
        // Given a raw meta data string of invalid data
        const AZStd::string rawTestTargetMetaData = "abcde";

        try
        {
            // When attempting to construct the test target
            const TestImpact::TestTargetMetaMap testTargetMetaData = TestImpact::TestTargetMetaMapFactory(rawTestTargetMetaData);

            // Do not expect this statement to be reachable
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::ArtifactException& e)
        {
            // Expect an artifact exception
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(TestTargetMetaMapFactoryTest, EmptyTestMetaArray_ExpectArtifactException)
    {
        // Given a raw meta data string of valid JSON that contains no tests
        const AZStd::string rawTestTargetMetaData =
            "{"
            "  \"google\": {"
            "    \"test\": {"
            "      \"tests\": ["
            "      ]"
            "    }"
            "  }"
            "}";

        try
        {
            // When attempting to construct the test target
            const TestImpact::TestTargetMetaMap testTargetMetaData = TestImpact::TestTargetMetaMapFactory(rawTestTargetMetaData);

            // Do not expect this statement to be reachable
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::ArtifactException& e)
        {
            // Expect an artifact exception
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(TestTargetMetaMapFactoryTest, EmptyName_ExpectArtifactException)
    {
        // Given a raw meta data string with a test that has no name value
        const AZStd::string rawTestTargetMetaData =
            "{"
            "  \"google\": {"
            "    \"test\": {"
            "      \"tests\": ["
            "        { \"name\": \"\", \"namespace\": \"Legacy\", \"suite\": \"main\", \"launch_method\": \"test_runner\" }"
            "      ]"
            "    }"
            "  }"
            "}";

        try
        {
            // When attempting to construct the test target
            const TestImpact::TestTargetMetaMap testTargetMetaData = TestImpact::TestTargetMetaMapFactory(rawTestTargetMetaData);

            // Do not expect this statement to be reachable
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::ArtifactException& e)
        {
            // Expect an artifact exception
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(TestTargetMetaMapFactoryTest, EmptyBuildType_ExpectArtifactException)
    {
        // Given a raw meta data string with a test that has no build type
        const AZStd::string rawTestTargetMetaData =
            "{"
            "  \"google\": {"
            "    \"test\": {"
            "      \"tests\": ["
            "        { \"name\": \"TestName\", \"namespace\": \"Legacy\", \"suite\": \"main\", \"launch_method\": \"\" }"
            "      ]"
            "    }"
            "  }"
            "}";

        try
        {
            // When attempting to construct the test target
            const TestImpact::TestTargetMetaMap testTargetMetaData = TestImpact::TestTargetMetaMapFactory(rawTestTargetMetaData);

            // Do not expect this statement to be reachable
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::ArtifactException& e)
        {
            // Expect an artifact exception
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(TestTargetMetaMapFactoryTest, InvalidBuildType_ExpectArtifactException)
    {
        // Given a raw meta data string with a test that has an invalid build type
        const AZStd::string rawTestTargetMetaData =
            "{"
            "  \"google\": {"
            "    \"test\": {"
            "      \"tests\": ["
            "        { \"name\": \"TestName\", \"namespace\": \"Legacy\", \"suite\": \"main\", \"launch_method\": \"Unknown\" }"
            "      ]"
            "    }"
            "  }"
            "}";

        try
        {
            // When attempting to construct the test target
            const TestImpact::TestTargetMetaMap testTargetMetaData = TestImpact::TestTargetMetaMapFactory(rawTestTargetMetaData);

            // Do not expect this statement to be reachable
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::ArtifactException& e)
        {
            // Expect an artifact exception
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(TestTargetMetaMapFactoryTest, ValidMetaData_ExpectValidTestMetaData)
    {
        // Given a raw meta data string valid test meta-data
        const AZStd::string rawTestTargetMetaData =
        "{"
            "  \"google\": {"
            "    \"test\": {"
            "      \"tests\": ["
            "        { \"name\": \"TestA\", \"namespace\": \"Legacy\", \"suite\": \"main\", \"launch_method\": \"test_runner\" },"
            "        { \"name\": \"TestB\", \"namespace\": \"\", \"suite\": \"main\", \"launch_method\": \"test_runner\" },"
            "        { \"name\": \"TestC\", \"namespace\": \"\", \"suite\": \"\", \"launch_method\": \"test_runner\" },"
            "        { \"name\": \"TestD\", \"namespace\": \"Legacy\", \"suite\": \"main\", \"launch_method\": \"stand_alone\" }"
            "      ]"
            "    }"
            "  }"
            "}";

        // When attempting to construct the test target
        const auto testTargetMetaData = TestImpact::TestTargetMetaMapFactory(rawTestTargetMetaData);

        // Expect the constructed test meta-data to match that of the supplied raw data
        EXPECT_EQ(testTargetMetaData.size(), 4);
        EXPECT_TRUE(testTargetMetaData.find("TestA") != testTargetMetaData.end());
        EXPECT_TRUE((testTargetMetaData.at("TestA") == TestImpact::TestTargetMeta{"main", TestImpact::LaunchMethod::TestRunner}));
        EXPECT_TRUE(testTargetMetaData.find("TestB") != testTargetMetaData.end());
        EXPECT_TRUE((testTargetMetaData.at("TestB") == TestImpact::TestTargetMeta{"main", TestImpact::LaunchMethod::TestRunner}));
        EXPECT_TRUE(testTargetMetaData.find("TestC") != testTargetMetaData.end());
        EXPECT_TRUE((testTargetMetaData.at("TestC") == TestImpact::TestTargetMeta{"", TestImpact::LaunchMethod::TestRunner}));
        EXPECT_TRUE(testTargetMetaData.find("TestD") != testTargetMetaData.end());
        EXPECT_TRUE((testTargetMetaData.at("TestD") == TestImpact::TestTargetMeta{"main", TestImpact::LaunchMethod::StandAlone}));
    }
} // namespace UnitTest
