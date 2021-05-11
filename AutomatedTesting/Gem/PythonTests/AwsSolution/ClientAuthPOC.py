"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

A sanity test for the built-in fixtures.
Launch the windows launcher attached to the currently installed instance.
"""
import logging
import pytest
import time

import ly_test_tools
import ly_remote_console.remote_console_commands
import ly_test_tools.log.log_monitor as log_monitor
import fixtures.asset_processor_fixture as ap
import ly_test_tools.o3de.asset_processor as asset_processor_commands

AWS_PROJECT_NAME = 'AWS-TEST'
AWS_CLIENTAUTH_FEATURE_NAME = 'AWSClientAuth'
AWS_CLIENTAUTH_DEFAULT_REGION = 'us-west-2'
AWS_CLIENTAUTH_DEFAULT_PROFILE_NAME = 'default'
AWS_CLIENTAUTH_DEFAULT_ACCOUNT_ID = ''

pytestmark = pytest.mark.SUITE_smoke

logger = logging.getLogger(__name__)


@pytest.fixture
def asset_processor(request: pytest.fixture, workspace: pytest.fixture) -> asset_processor_commands.AssetProcessor:
    """
    Sets up usage of the asset proc
    :param request:
    :return: ly_test_tools.03de.asset_processor.AssetProcessor
    """

    # Initialize the Asset Processor
    ap = asset_processor_commands.AssetProcessor(workspace)

    # Custom teardown method for this fixture
    def teardown():
        logger.info("Running asset_processor_fixture teardown to stop the AP")
        ap.stop()

    request.addfinalizer(teardown)

    return ap


@pytest.mark.usefixtures("automatic_process_killer")
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['AWSClientAuthTest'])
class TestAutomatedClientAuth(object):

    @pytest.mark.parametrize('launcher_platform', ['windows'])
    def test_load_level(self, project, launcher, level, launcher_platform, request):
        monitor = log_monitor.LogMonitor(launcher, '../../../user/log/Game.log')
        expected_lines = ["Successfully opened document C:/Users/clujames/GitRepo/o3de/AutomatedTesting/Levels/AWSClientAuthTest/AWSClientAuthTest.ly"]

        def teardown():
            launcher.stop()

        request.addfinalizer(teardown)

        with launcher.start():
            try:
                # give it a few seconds to load up
                time.sleep(2)
                result = monitor.monitor_log_for_lines(expected_lines)

                assert result
            except log_monitor.LogMonitorException as e:
                print(e)
                assert False, f"A log monitor error occurred: {e}"

    @pytest.mark.usefixtures("asset_processor")
    @pytest.mark.parametrize('launcher_platform', ['windows'])
    def test_notification_bus_initialization(self, launcher, launcher_platform, level, project, request, asset_processor):
        def teardown():
            launcher.stop()
            asset_processor.stop()

        request.addfinalizer(teardown)

        monitor = log_monitor.LogMonitor(launcher,
                                         "'../../../user/log/Game.log'")
        expected_lines = ["(Script) - Successfully started Cognito Authorization Bus",
                          "(Script) - Successfully started Cognito User Management Bus",
                          "(Script) - Successfully started Authentication Provider Bus"]
        unexpected_lines = ["(Script) - Failed to start Cognito User Management Bus",
                            "(Script) - Failed to start the Cognito Authorization Bus",
                            "(Script) - Failed to start the Authentication Provider Bus"]

        with asset_processor.start():
            with launcher.start():
                result = monitor.monitor_log_for_lines(expected_lines, unexpected_lines)
                assert result, "One or more of the ClientAuth buses failed to start via script."

    #  @pytest.mark.parametrize('project', ['AutomatedTesting'])
    # @pytest.mark.parametrize('level', ['AWSClientAuthTest'])
    @pytest.mark.parametrize('launcher_platform', ['windows'])
    @pytest.mark.test_case_id('C35933392')
    def test_refresh_tokens(self, launcher, launcher_platform, level, project, request):
        remote_console = ly_remote_console.remote_console_commands.RemoteConsole()

        def teardown():
            remote_console.stop()
            launcher.stop()

        request.addfinalizer(teardown)

        with launcher.start():
            remote_console.start()
            time.sleep(5)
            result = remote_console.expect_log_line("(Script) - Refresh token success!")
        assert result, "Could not find the following line: (Script) - Refresh token success!"

