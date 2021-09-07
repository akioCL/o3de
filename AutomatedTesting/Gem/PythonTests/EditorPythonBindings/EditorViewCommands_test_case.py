"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Tests a portion of the Editor View Python API from CryEdit.cpp while the Editor is running

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.math
import azlmbr.legacy.general as general

# Open a level (any level should work)
import azlmbr.legacy.general as general
editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'OpenLevelNoPrompt', 'auto_test')
general.idle_wait(2.0)

def fetch_vector3_parts(vec3):
    x = vec3.get_property('x')
    y = vec3.get_property('y')
    z = vec3.get_property('z')
    return (x, y, z)

pos = general.get_current_view_position()
rot = general.get_current_view_rotation()

px, py, pz = fetch_vector3_parts(pos)
rx, ry, rz = fetch_vector3_parts(rot)

p1x = px + 5.0
p1y = py - 2.0
p1z = pz + 1.0

r1x = rx + 2.0
r1y = ry + 3.0
r1z = rz - 5.0

general.set_current_view_position(p1x, p1y, p1z)
general.set_current_view_rotation(r1x, r1y, r1z)

pos2 = general.get_current_view_position()
rot2 = general.get_current_view_rotation()

p2x, p2y, p2z = fetch_vector3_parts(pos2)
r2x, r2y, r2z = fetch_vector3_parts(rot2)

if not (px == p2x) and not (py == p2y) and not (pz == p2z):
    print("set_current_view_position works")

if not (rx == r2x) and not (ry == r2y) and not (rz == r2z):
    print("set_current_view_rotation works")

editor.EditorToolsApplicationRequestBus(bus.Broadcast, 'ExitNoPrompt')
