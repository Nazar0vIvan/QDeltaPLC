"""Geometry-only properties of touch plans (no random sampling):
- check of formula (3.2) of the calibration document on the 37-touch table;
- A5 range per group of the group plans;
- transfer of touch noise into the estimated ball centre: sqrt(diag((J^T J)^-1)) per 1 um,
  correlation of the along-stylus ball coordinate with the contact distance, condition number.
Run from this folder: python conditioning.py"""
import numpy as np
from kr10 import D2R
from plans import TOUCHES, touch_rotation, normal, plan_current, plan_groups, GROUPS_PROPOSED
from lsq import NOM, nominal_joints, fit7

G_MOD = [(0, 0, 0), (0, 0, 90), (0, 0, 180), (0, 0, 270)] + [(30, g, 0) for g in (240, 270, 300, 0, 60, 90)]
PLANS = {'current37': plan_current(), 'groups10': plan_groups(GROUPS_PROPOSED), 'groups10mod': plan_groups(G_MOD)}

err = 0.0
for lat, lon, tilt, roll in TOUCHES:
    R = touch_rotation(lat, lon, tilt, roll)
    t, p = tilt * D2R, roll * D2R
    err = max(err, np.abs(R.T @ (-normal(lat, lon))
                          - np.array([-np.sin(t) * np.cos(p), np.sin(t) * np.sin(p), np.cos(t)])).max())
print("max deviation of (3.2) over 37 touches: %.1e" % err)

for name, plan in PLANS.items():
    Q = nominal_joints(plan)
    if name != 'current37':
        groups = sorted({g for _, _, g in plan})
        for g in groups:
            a5 = [q[4] for q, (_, _, gg) in zip(Q, plan) if gg == g]
            print("  %s group %d: A5 %.1f..%.1f deg" % (name, g + 1, min(a5), max(a5)))
    x, e, J = fit7([NOM.fk(q) for q in Q])
    C = np.linalg.inv(J.T @ J)
    sd = np.sqrt(np.diag(C))
    Js = J / np.linalg.norm(J, axis=0)
    print("%s m=%d  sigma(ball x,y,z; rho) per 1 um: %s  corr(ball z, rho)=%.3f  cond=%.1f"
          % (name, len(Q), np.round(sd[[0, 1, 2, 6]], 3), C[2, 6] / np.sqrt(C[2, 2] * C[6, 6]),
             np.linalg.cond(Js)))
