"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de import launcher_test
import ly_remote_console.remote_console_commands as rc


@pytest.fixture
def remote_console_instance(request):
    console = rc.RemoteConsole()

    def teardown():
        if console.connected:
            console.stop()

    request.addfinalizer(teardown)
    return console


@pytest.mark.parametrize("launcher_platform", ['windows'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestLauncherAutomation:

    @pytest.mark.parametrize("level", ["levels/DynamicVegetationLauncherTests/DynamicVegetationLauncherTests.spawnable"])
    def test_DynVeg_LauncherTests(self, launcher, level, remote_console_instance, launcher_platform):
        launcher_test.run_launcher_tests(launcher, level, remote_console_instance, launch_ap=False)
