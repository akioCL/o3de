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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <TestImpactFramework/TestImpactException.h>

namespace UnitTest
{
    constexpr const char* Message = "String Constructor";

    TEST(ExceptionTest, DefaultConstructor_HasEmptyMessageString)
    {
        // Given an exception instantiated with the default constructor
        const TestImpact::Exception e;

        // Expect the message to be an empty string
        EXPECT_STREQ(e.what(), "\0");
    }

    TEST(ExceptionTest, StringConstructor_HasSpecifiedMessageString)
    {
        // Given an exception instantiated with a string
        const AZStd::string msg(Message);
        const TestImpact::Exception e(msg);

        // Expect the message to be the specified string
        EXPECT_STREQ(e.what(), Message);
    }

    TEST(ExceptionTest, StringLiteralConstructor_HasSpecifiedMessageString)
    {
        // Given an exception instantiated with a string literal
        const TestImpact::Exception e(Message);

        // Expect the message to be the specified string
        EXPECT_STREQ(e.what(), Message);
    }

    TEST(ExceptionTest, InitializedWithLocalString_HasCopyOfLocalStringString)
    {
        try
        {
            // Given an exception instantiated with a string in local scope
            throw(TestImpact::Exception(AZStd::string::format("%s", Message)));

            // Do not expect this code to be reachable
            FAIL();
        }
        catch (const TestImpact::Exception& e)
        {
            // Expect the message to be the specified out-of-scope string
            EXPECT_STREQ(e.what(), Message);
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(ExceptionTest, InitializedWithLocalStringLiteral_HasCopyOfLocalStringString)
    {
        try
        {
            // Given an exception instantiated with a copy of the message string in local scope
            throw(TestImpact::Exception(AZStd::string::format("%s", Message).c_str()));

            // Do not expect this code to be reachable
            FAIL();
        }
        catch (const TestImpact::Exception& e)
        {
            // Expect the message to be the specified out-of-scope string literal
            EXPECT_STREQ(e.what(), Message);
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(ExceptionTest, EvalMacroFails_ExceptionTestThrown)
    {
        try
        {
            // Given an exception instantiated with a string literal in local scope
            AZ_TestImpact_Eval(false, TestImpact::Exception, Message);

            // Do not expect this code to be reachable
            FAIL();
        }
        catch (const TestImpact::Exception& e)
        {
            // Expect the message contain the specified string
            EXPECT_STREQ(e.what(), Message);
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST(ExceptionTest, EvalMacroSucceeds_ExceptionTestNotThrown)
    {
        try
        {
            // Given an exception instantiated with a string literal in local scope
            AZ_TestImpact_Eval(true, TestImpact::Exception, Message);

            // Expect this code to be reachable
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any exceptions
            FAIL();
        }
    }

    TEST(ExceptionTest, Throw_ExceptionTestThrown)
    {
        try
        {
            // Given an exception instantiated with a string literal in local scope
            throw(TestImpact::Exception(Message));
        }
        catch (const TestImpact::Exception& e)
        {
            // Expect the message contain the specified string
            EXPECT_STREQ(e.what(), Message);
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }
} // namespace UnitTest
