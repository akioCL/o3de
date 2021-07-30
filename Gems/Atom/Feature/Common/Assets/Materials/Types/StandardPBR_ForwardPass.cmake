# 
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or 
# its licensors. 
# 
# For complete copyright and license terms please see the LICENSE at the root of this 
# distribution (the "License"). All use of this software is governed by the License, 
# or, if provided, by the license below or the license accompanying this file. Do not 
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS, 
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
# 

ly_add_shader_variant_list(
	NAME StandardPBR_ForwardPass
	STABLE_IDS 
		1
		2 
		3 
		4
	OPTIONS 
		"o_directional_shadow_filtering_method=ShadowFilterMethod::None"
		"o_directional_shadow_filtering_method=ShadowFilterMethod::Pcf"
		"o_directional_shadow_filtering_method=ShadowFilterMethod::Esm"
		"o_directional_shadow_filtering_method=ShadowFilterMethod::EsmPcf"
	ENTRY_VS StandardPbr_ForwardPassVS
	ENTRY_PS StandardPbr_ForwardPassPS
)