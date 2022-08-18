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
"""! A module to discover installed versions of wing pro for selection

:file: < DCCsi >/Tools/IDE/Wing/discovery.py
:Status: Prototype, very barebones
:Version: 0.0.1
Future: provide a variety of ways to discover wing, i.e. use winreg
"""
# -------------------------------------------------------------------------
# standard imports
from pathlib import Path
from typing import Union
import logging as _logging
# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'DCCsi.Tools.IDE.Wing.discovery'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------
# dccsi imports
# the default and currently only supported discovery path
from DccScriptingInterface.Tools.IDE.Wing.constants import PATH_WINGHOME
# -------------------------------------------------------------------------
def get_default_install(winghome: Union[str, Path] = PATH_WINGHOME) -> Path:
    """! validates then returns the default dccsi winghome
    :return: Path(winghome), or None"""
    winghome = Path(winghome).resolve()
    if winghome.exists():
        return winghome
    else:
        _LOGGER.error(f'WINGHOME not valid: {winghome.as_posix()}')
        return None

def find_all_installs() -> list:
    """! finds all platform installations of wing
    :return: list of wing install locations"""
    # ! this method is not implemented, future work.
    return list()
# --- END -----------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """! Run this file as main, local testing"""

    winghome = get_default_install(winghome=PATH_WINGHOME)

    if winghome.exists():
        _LOGGER.info(f'WINGHOME: {winghome}')
    else:
        _LOGGER.error(f'WINGHOME not valid: {winghome.as_posix()}')