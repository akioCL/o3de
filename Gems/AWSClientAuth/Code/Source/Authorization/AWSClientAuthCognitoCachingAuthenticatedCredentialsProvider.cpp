/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Authorization/AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider.h>

#include <AzCore/Debug/Trace.h>

#include <aws/cognito-identity/CognitoIdentityClient.h>
#include <aws/cognito-identity/model/GetCredentialsForIdentityRequest.h>
#include <aws/cognito-identity/model/GetIdRequest.h>
#include <aws/core/utils/Outcome.h>
#include <aws/identity-management/auth/CognitoCachingCredentialsProvider.h>
#include <aws/identity-management/auth/PersistentCognitoIdentityProvider.h>


namespace AWSClientAuth
{
    // mostly borrowed from:
    // https://github.com/aws/aws-sdk-cpp/blob/main/aws-cpp-sdk-identity-management/source/auth/CognitoCachingCredentialsProvider.cpp#L92
    Aws::CognitoIdentity::Model::GetCredentialsForIdentityOutcome FetchCredsFromCognito(
        const Aws::CognitoIdentity::CognitoIdentityClient& cognitoIdentityClient,
        Aws::Auth::PersistentCognitoIdentityProvider& identityRepository,
        const char* logTag,
        bool includeLogins)
    {
        auto logins = identityRepository.GetLogins();
        Aws::Map<Aws::String, Aws::String> cognitoLogins;
        for (auto& login : logins)
        {
            cognitoLogins[login.first] = login.second.accessToken;
        }

        if (!identityRepository.HasIdentityId())
        {
            auto accountId = identityRepository.GetAccountId();
            auto identityPoolId = identityRepository.GetIdentityPoolId();

            Aws::CognitoIdentity::Model::GetIdRequest getIdRequest;
            getIdRequest.SetIdentityPoolId(identityPoolId);
            if (!accountId.empty()) // new check
            {
                getIdRequest.SetAccountId(accountId);
                AZ_Printf(
                    logTag, "Identity not found, requesting an id for accountId %s identity pool id %s",
                    accountId, identityPoolId)
            }
            else
            {
                AZ_Printf(logTag, "Identity not found, requesting an id for identity pool id %s", identityPoolId)
            }
            if (includeLogins)
            {
                getIdRequest.SetLogins(cognitoLogins);
            }

            auto getIdOutcome = cognitoIdentityClient.GetId(getIdRequest);
            if (getIdOutcome.IsSuccess())
            {
                auto identityId = getIdOutcome.GetResult().GetIdentityId();
                AZ_Printf(logTag, "Successfully retrieved identity: %s", identityId)
                identityRepository.PersistIdentityId(identityId);
            }
            else
            {
                AZ_Error(
                    logTag, false, "Unable to get Identity ID from Cognito: %s %s",
                    getIdOutcome.GetError().GetExceptionName(),
                    getIdOutcome.GetError().GetMessage())
                return Aws::CognitoIdentity::Model::GetCredentialsForIdentityOutcome(getIdOutcome.GetError());
            }
        }

        Aws::CognitoIdentity::Model::GetCredentialsForIdentityRequest getCredentialsForIdentityRequest;
        getCredentialsForIdentityRequest.SetIdentityId(identityRepository.GetIdentityId());
        if (includeLogins)
        {
            getCredentialsForIdentityRequest.SetLogins(cognitoLogins);
        }

        return cognitoIdentityClient.GetCredentialsForIdentity(getCredentialsForIdentityRequest);
    }

    // base / auth
    AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider::AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider(
        const std::shared_ptr<Aws::Auth::PersistentCognitoIdentityProvider>& identityRepository,
        const std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient>& cognitoIdentityClient)
        : CognitoCachingCredentialsProvider(identityRepository, cognitoIdentityClient)
    {
    }

    static const char* AUTH_LOG_TAG = "AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider";

    Aws::CognitoIdentity::Model::GetCredentialsForIdentityOutcome
    AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider::GetCredentialsFromCognito() const
    {
        // true = AuthenticatedCredsProvider
        return FetchCredsFromCognito(*m_cognitoIdentityClient, *m_identityRepository, AUTH_LOG_TAG, true);
    }

    // anon
    AWSClientAuthCachingAnonymousCredsProvider::AWSClientAuthCachingAnonymousCredsProvider(
        const std::shared_ptr<Aws::Auth::PersistentCognitoIdentityProvider>& identityRepository,
        const std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient>& cognitoIdentityClient)
        : AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider(identityRepository, cognitoIdentityClient)
    {
    }

    static const char* ANON_LOG_TAG = "AWSClientAuthCachingAnonymousCredsProvider";

    Aws::CognitoIdentity::Model::GetCredentialsForIdentityOutcome AWSClientAuthCachingAnonymousCredsProvider::
        GetCredentialsFromCognito() const
    {
        // false = AnonymousCredentialsProvider
        return FetchCredsFromCognito(*m_cognitoIdentityClient, *m_identityRepository, ANON_LOG_TAG, false);
    }


} // namespace AWSClientAuth
