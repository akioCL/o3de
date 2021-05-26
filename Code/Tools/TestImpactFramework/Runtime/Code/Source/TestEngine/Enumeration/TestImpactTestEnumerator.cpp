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

#include <TestImpactFramework/TestImpactUtils.h>

#include <Artifact/Factory/TestImpactTestEnumerationSuiteFactory.h>
#include <TestEngine/TestImpactTestEngineException.h>
#include <TestEngine/Enumeration/TestImpactTestEnumerationSerializer.h>
#include <TestEngine/Enumeration/TestImpactTestEnumerator.h>
#include <TestEngine/Enumeration/TestImpactTestEnumeration.h>

#include <AzCore/IO/SystemFile.h>

namespace TestImpact
{
    TestEnumeration ParseTestEnumerationFile(const RepoPath& enumerationFile)
    {
        return TestEnumeration(GTest::TestEnumerationSuitesFactory(ReadFileContents<TestEngineException>(enumerationFile)));
    }

    TestEnumerationJobData::TestEnumerationJobData(const RepoPath& enumerationArtifact, AZStd::optional<Cache>&& cache)
        : m_enumerationArtifact(enumerationArtifact)
        , m_cache(AZStd::move(cache))
    {
    }

    const RepoPath& TestEnumerationJobData::GetEnumerationArtifactPath() const
    {
        return m_enumerationArtifact;
    }

    const AZStd::optional<TestEnumerationJobData::Cache>& TestEnumerationJobData::GetCache() const
    {
        return m_cache;
    }

    TestEnumerator::TestEnumerator(size_t maxConcurrentEnumerations)
        : JobRunner(maxConcurrentEnumerations)
    {
    }
    
    AZStd::pair<ProcessSchedulerResult, AZStd::vector<TestEnumerator::Job>> TestEnumerator::Enumerate(
        const AZStd::vector<JobInfo>& jobInfos,
        JobExceptionPolicy jobExceptionPolicy,
        AZStd::optional<AZStd::chrono::milliseconds> enumerationTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> enumeratorTimeout,
        AZStd::optional<ClientJobCallback> clientCallback)
    {
        AZStd::vector<Job> cachedJobs;
        AZStd::vector<JobInfo> jobQueue;

        for (const auto& jobInfo : jobInfos)
        {
            // If this job has a cache read policy attempt to read the cache
            if (jobInfo.GetCache().has_value())
            {
                if (jobInfo.GetCache()->m_policy == JobData::CachePolicy::Read)
                {
                    JobMeta meta;
                    AZStd::optional<TestEnumeration> enumeration;

                    try
                    {
                        enumeration = TestEnumeration(DeserializeTestEnumeration(ReadFileContents<TestEngineException>(jobInfo.GetCache()->m_file)));
                    }
                    catch (const TestEngineException& e)
                    {
                        AZ_Printf("Enumerate", "Enumeration cache error: %s", e.what());
                        DeleteFiles(jobInfo.GetCache()->m_file.ParentPath(), jobInfo.GetCache()->m_file.Filename().Native());
                    }

                    // Even though cached jobs don't get executed we still give the client the opportunity to handle the job state
                    // change in order to make the caching process transparent to the client
                    if (enumeration.has_value())
                    {
                        if (m_clientJobCallback.has_value())
                        {
                            (*m_clientJobCallback)(jobInfo, meta);
                        }

                        // Cache read successfully, this job will not be placed in the job queue
                        cachedJobs.emplace_back(Job(jobInfo, AZStd::move(meta), AZStd::move(enumeration)));
                    }
                    else
                    {
                        // The cache read failed and exception policy for cache read failures is not to throw so instead place this job in the
                        // job queue
                        jobQueue.emplace_back(jobInfo);
                    }
                }
                else
                {
                    // This job has no cache read policy so delete the cache and place in job queue
                    DeleteFiles(jobInfo.GetCache()->m_file.ParentPath(), jobInfo.GetCache()->m_file.Filename().Native());
                    jobQueue.emplace_back(jobInfo);
                }
            }
            else
            {
                // This job has no cache read policy so place in job queue
                jobQueue.emplace_back(jobInfo);
            }
        }

        const auto payloadGenerator = [this](const JobDataMap& jobDataMap)
        {
            PayloadMap<Job> enumerations;
            for (const auto& [jobId, jobData] : jobDataMap)
            {
                const auto& [meta, jobInfo] = jobData;
                if (meta.m_result == JobResult::ExecutedWithSuccess)
                {
                    try
                    {
                        const auto& enumeration = (enumerations[jobId] = ParseTestEnumerationFile(jobInfo->GetEnumerationArtifactPath()));

                        // Write out the enumeration to a cache file if we have a cache write policy for this job
                        if (jobInfo->GetCache().has_value() && jobInfo->GetCache()->m_policy == JobData::CachePolicy::Write)
                        {
                            WriteFileContents<TestEngineException>(SerializeTestEnumeration(enumeration.value()), jobInfo->GetCache()->m_file);
                        }
                    }
                    catch (const Exception& e)
                    {
                        AZ_Warning("Enumerate", false, e.what());
                        enumerations[jobId] = AZStd::nullopt;
                    }
                }
            }

            return enumerations;
        };

        // Generate the enumeration results for the jobs that weren't cached
        auto [result, jobs] = ExecuteJobs(
            jobQueue,
            jobExceptionPolicy,
            payloadGenerator,
            StdOutputRouting::None,
            StdErrorRouting::None,
            enumerationTimeout,
            enumeratorTimeout,
            clientCallback,
            AZStd::nullopt);
       
        // We need to add the cached jobs to the completed job list even though they technically weren't executed
        for (auto&& job : cachedJobs)
        {
            jobs.emplace_back(AZStd::move(job));
        }

        return { result, jobs };
    }
} // namespace TestImpact
