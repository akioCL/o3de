# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! O3DE DCCsi Wing Pro 8+ IDE start module

:file: < DCCsi >/Tools/IDE/Wing/start.py
:Status: Prototype
:Version: 0.0.1

Notice: This module is an entrypoint and has a cli
"""
# -------------------------------------------------------------------------
import timeit
_MODULE_START = timeit.default_timer()  # start tracking

# standard imports
import sys
import os
import site
import subprocess
from pathlib import Path
import logging as _logging
# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'DCCsi.Tools.IDE.Wing.start'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug(f'Initializing: {_MODULENAME}')

_MODULE_PATH = Path(__file__)

# ensure code access to the DCCsi
from DccScriptingInterface.Tools import PATH_DCCSIG

# initialize the wing config and settings
import DccScriptingInterface.Tools.IDE.Wing.config as wing_config
_SETTINGS = wing_config.get_config_settings()
# --- END -----------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main (external commandline)"""

    _MODULENAME = f'{_MODULENAME}.cli'

    from DccScriptingInterface.globals import *

    from DccScriptingInterface.constants import STR_CROSSBAR
    from DccScriptingInterface.constants import FRMT_LOG_LONG

    # configure basic logger
    # note: not using a common logger to reduce cyclical imports
    _logging.basicConfig(level=DCCSI_LOGLEVEL,
                         format=FRMT_LOG_LONG,
                         datefmt='%m-%d %H:%M')

    _LOGGER = _logging.getLogger(_MODULENAME)

    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.debug(f'_MODULENAME: {_MODULENAME}')
    _LOGGER.debug(f'{ENVAR_DCCSI_GDEBUG}: {DCCSI_GDEBUG}')
    _LOGGER.debug(f'{ENVAR_DCCSI_DEV_MODE}: {DCCSI_DEV_MODE}')
    _LOGGER.debug(f'{ENVAR_DCCSI_GDEBUGGER}: {DCCSI_GDEBUGGER}')
    _LOGGER.debug(f'{ENVAR_DCCSI_LOGLEVEL}: {DCCSI_LOGLEVEL}')
    _LOGGER.debug(f'{ENVAR_DCCSI_TESTS}: {DCCSI_TESTS}')

    # commandline interface
    import argparse
    parser = argparse.ArgumentParser(
        description=f'O3DE {_MODULENAME}',
        epilog="Attempts to start Wing Pro 8+ with the DCCsi and O3DE bootstrapping")

    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables global debug flag.')

    parser.add_argument('-dm', '--developer-mode',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables dev mode for early auto attaching debugger.')

    parser.add_argument('-sd', '--set-debugger',
                        type=str,
                        required=False,
                        default='WING',
                        help='Default debugger: WING, (not implemented) others: PYCHARM and VSCODE.')

    parser.add_argument('-ex', '--exit',
                        type=bool,
                        required=False,
                        default=False,
                        help='Exits python. Do not exit if you want to be in interactive interpreter after config')

    args = parser.parse_args()

    # easy overrides
    if args.global_debug:
        _DCCSI_GDEBUG = True
        os.environ["DYNACONF_DCCSI_GDEBUG"] = str(_DCCSI_GDEBUG)

    if args.developer_mode:
        from azpy.config_utils import attach_debugger
        _DCCSI_DEV_MODE = True
        attach_debugger()  # attempts to start debugger

    if args.set_debugger:
        _LOGGER.info('Setting and switching debugger type not implemented (default=WING)')
        # To Do: implement debugger plugin pattern

    try:
        subprocess.Popen(f'{_SETTINGS.DCCSI_SUBSTANCE_EXE}',
                         env=os.environ.copy(),
                         shell=True)
    except Exception as e:
        _LOGGER.error(f'Could not start Wing: {e}')

    # -- DONE ----
    _LOGGER.info(STR_CROSSBAR)

    _LOGGER.debug('{0} took: {1} sec'.format(_MODULENAME, timeit.default_timer() - _MODULE_START))

    if args.exit:
        import sys
        # return
        sys.exit()
    else:
        # custom prompt
        sys.ps1 = "[{}]>>".format(_MODULENAME)
# --- END -----------------------------------------------------------------