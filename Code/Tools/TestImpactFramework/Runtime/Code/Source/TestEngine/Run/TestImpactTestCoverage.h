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

#pragma once

#include <Artifact/Dynamic/TestImpactCoverage.h>

#include <AzCore/std/containers/set.h>

namespace TestImpact
{
    //! Scope of coverage data.
    enum class CoverageLevel : bool
    {
        Source, //!< Line-level coverage data.
        Line //!< Source-level coverage data.
    };

    //! Representation of a given test target's test coverage results.
    class TestCoverage
    {
    public:
        TestCoverage(AZStd::vector<ModuleCoverage>&& moduleCoverages);

        //! Returns the number of unique sources covered.
        size_t GetNumSourcesCovered() const;

        //! Returns the number of modules (dynamic libraries, child processes, etc.) covered.
        size_t GetNumModulesCovered() const;

        //! Returns the sorted set of unique sources covered (empty if no coverage).
        const AZStd::vector<AZStd::string>& GetSourcesCovered() const;

        //! Returns the modules covered (empty if no coverage).
        const AZStd::vector<ModuleCoverage>& GetModuleCoverages() const;

        //! Returns the coverage level (empty if no coverage).
        AZStd::optional<CoverageLevel> GetCoverageLevel() const;

    private:
        AZStd::vector<ModuleCoverage> m_modules;
        AZStd::vector<AZStd::string> m_sourcesCovered;
        AZStd::optional<CoverageLevel> m_coverageLevel;
    };
} // namespace TestImpact
