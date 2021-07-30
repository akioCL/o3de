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

ly_add_shader(
	NAME StandardPBR_ForwardPass
	AZSL Materials/Types/StandardPBR_ForwardPass.azsl
	SHADER_VARIANTS Materials/Types/StandardPBR_ForwardPass.cmake
	DEPTH_STENCIL_STATE "{
        \"Depth\" :
        {
            \"Enable\" : true,
            \"CompareFunc\" : \"GreaterEqual\"
        },
        \"Stencil\" :
        {
            \"Enable\" : true,
            \"ReadMask\" : \"0x00\",
            \"WriteMask\" : \"0xFF\",
            \"FrontFace\" :
            {
                \"Func\" : \"Always\",
                \"DepthFailOp\" : \"Keep\",
                \"FailOp\" : \"Keep\",
                \"PassOp\" : \"Replace\"
            },
            \"BackFace\" :
            {
                \"Func\" : \"Always\",
                \"DepthFailOp\" : \"Keep\",
                \"FailOp\" : \"Keep\",
                \"PassOp\" : \"Replace\"
            }
        }
    },"
	ENTRY_VS StandardPbr_ForwardPassVS
	ENTRY_PS StandardPbr_ForwardPassPS
)