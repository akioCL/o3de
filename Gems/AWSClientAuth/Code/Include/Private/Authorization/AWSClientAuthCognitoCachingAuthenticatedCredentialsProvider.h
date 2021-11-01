/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <aws/cognito-identity/CognitoIdentityClient.h>
#include <aws/identity-management/auth/CognitoCachingCredentialsProvider.h>
#include <aws/identity-management/auth/PersistentCognitoIdentityProvider.h>

namespace AWSClientAuth
{
    // base
    class AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider
        : public Aws::Auth::CognitoCachingCredentialsProvider
    {
    public:
        AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider(
            const std::shared_ptr<Aws::Auth::PersistentCognitoIdentityProvider>& identityRepository,
            const std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient>& cognitoIdentityClient = nullptr);

    protected:
        Aws::CognitoIdentity::Model::GetCredentialsForIdentityOutcome GetCredentialsFromCognito() const override;
    };

    // anon creds
    class AWSClientAuthCachingAnonymousCredsProvider : public AWSClientAuthCognitoCachingAuthenticatedCredentialsProvider
    {
    public:
        AWSClientAuthCachingAnonymousCredsProvider(
            const std::shared_ptr<Aws::Auth::PersistentCognitoIdentityProvider>& identityRepository,
            const std::shared_ptr<Aws::CognitoIdentity::CognitoIdentityClient>& cognitoIdentityClient = nullptr);

    protected:
        Aws::CognitoIdentity::Model::GetCredentialsForIdentityOutcome GetCredentialsFromCognito() const override;
    };

} // namespace AWSClientAuth
