/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#define AZ_SERIALIZE_SWAP_ENDIAN(_value, _isSwap)  if (_isSwap) AZStd::endian_swap(_value)
