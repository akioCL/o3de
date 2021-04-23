"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
LY-124059 : CLI tool - AssetBuilder
Launch AssetBuilder and Verify the help message
"""

import os
import pytest
import subprocess
import ly_test_tools.environment.process_utils as process_utils


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.SUITE_smoke
class TestAssetBuilder(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request):
        def teardown():
            process_utils.kill_processes_named("AssetBuilder", True)

        request.addfinalizer(teardown)

    @pytest.mark.test_case_id("LY-124059")
    def test_AssetBuilder(self, request, editor, build_directory):
        file_path = os.path.join(build_directory, "AssetBuilder")
        help_message = "AssetBuilder is part of the Asset Processor"
        # Launch AssetBuilder
        output = subprocess.run([file_path, "-help"], capture_output=True)
        assert (
            len(output.stderr) == 0 and output.returncode == 0
        ), f"Error occurred while launching {file_path}: {output.stderr}"
        # Verify help message
        assert help_message in str(output.stdout), f"Help Message: {help_message} is not present"
