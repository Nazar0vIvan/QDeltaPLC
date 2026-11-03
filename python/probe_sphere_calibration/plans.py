"""Touch plans: the current 37-touch table (probetouchplan.cpp) and the 10-group proposal
(plans/PROBE_SPHERE_CALIBRATION_GROUPS_RU.md, section 3). Each touch: normal n in {B} and
flange rotation R ({P} axes parallel to {F})."""
import numpy as np
from kr10 import rx, ry, rz, D2R

PS_NOM = np.array([1054.1403, -331.9692, 454.2727])
TCP_NOM = np.array([-0.048, -0.097, 127.0])
R_S, R_B = 12.5, 1.5
L = R_S + R_B
GAP = 3.0

TOUCHES = [
    (0, 180, 0, 30), (-30, 180, 30, 30), (-30, 216, 30, 0), (0, 210, 0, 0), (0, 240, 0, 0),
    (-30, 252, 30, 0), (-30, 288, 50, 0), (0, 270, 0, 0), (0, 300, 40, 30), (0, 330, 45, 60),
    (30, 342, 30, 60), (30, 18, 15, 90), (30, 54, 0, 120), (30, 90, 0, 120), (30, 126, 50, -150),
    (30, 162, 50, -120), (30, 198, 50, -90), (30, 234, 0, 0), (30, 270, 0, 0), (30, 306, 15, 30),
    (60, 300, 0, 30), (60, 0, 0, 90), (60, 60, 0, 150), (60, 120, 15, -150), (60, 180, 30, -90),
    (60, 240, 0, 0),
    (90, 0, 0, 90),
    (0, 0, 45, 90), (0, 30, 30, 120), (-30, 36, 55, 120), (0, 60, 15, 120), (-30, 72, 45, 120),
    (-30, 108, 45, 90), (0, 90, 15, 90), (0, 120, 15, 90), (-30, 144, 30, 60), (0, 150, 0, 60)]


def normal(lat, lon):
    la, lo = lat * D2R, lon * D2R
    return np.array([np.cos(la) * np.cos(lo), np.cos(la) * np.sin(lo), np.sin(la)])


def touch_rotation(lat, lon, tilt, roll):
    return rz(lon) @ ry(90 - lat - tilt) @ rx(180) @ rz(roll)


def plan_current():
    return [(normal(a, b), touch_rotation(a, b, c, d), 0) for a, b, c, d in TOUCHES]


def group_rotation(beta, gamma, psi):
    return rz(gamma) @ ry(beta) @ rx(180) @ rz(psi)


GROUP_DIRS = [(0.0, 0.0)] + [(45.0, p) for p in range(0, 360, 60)] + [(90.0, p) for p in range(30, 360, 60)]


def approach_in_P(tau, phi):
    t, f = tau * D2R, phi * D2R
    return np.array([np.sin(t) * np.cos(f), np.sin(t) * np.sin(f), np.cos(t)])


def plan_groups(groups):
    """groups: list of (beta, gamma, psi). Returns touches tagged with group index."""
    out = []
    for g, (b, c, p) in enumerate(groups):
        R = group_rotation(b, c, p)
        for tau, phi in GROUP_DIRS:
            n = -R @ approach_in_P(tau, phi)
            out.append((n, R, g))
    return out


GROUPS_PROPOSED = [(0, 0, 0), (0, 0, 90), (0, 0, 180), (0, 0, 270)] + [(30, g, 0) for g in range(0, 360, 60)]
