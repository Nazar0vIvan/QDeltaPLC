"""Nominal joint angles of touch plans and least-squares estimation of the ball centre.
Equations: docs/ROBOT_PROBE_SPHERE_CALIBRATION_RU.md, (5.4.3) with the contact distance estimated."""
import numpy as np
from kr10 import Robot, QMIN, QMAX, ik_ball
from plans import PS_NOM, TCP_NOM, L

NOM = Robot()
SEEDS = [np.array([20, -50, 85, a4, a5, a6], float)
         for a4 in (-150, -90, -30, 30, 90, 150) for a5 in (60, 100) for a6 in (-240, -120, 0, 120)]


def solve(robot, ball, R, tcp, prev):
    """IK in the A5 > 0 wrist within joint limits, nearest to prev."""
    best = None
    for s in ([prev] if prev is not None else []) + SEEDS:
        q = ik_ball(robot, ball, R, tcp, s)
        if q is None or np.any(q < QMIN - 1e-6) or np.any(q > QMAX + 1e-6) or q[4] <= 0:
            continue
        d = 0 if prev is None else np.abs(q - prev).max()
        if best is None or d < best[0]:
            best = (d, q)
        if prev is not None and s is prev and d < 20:
            break
    return None if best is None else best[1]


def nominal_joints(plan):
    """Joint angles at the nominal contact of each touch (plan order, previous touch as seed)."""
    out, prev = [], None
    for n, R, g in plan:
        q = solve(NOM, PS_NOM + L * n, R, TCP_NOM, prev)
        if q is None:
            raise RuntimeError("unreachable touch")
        out.append(q)
        prev = q
    return out


def fit7(F, x0=None, iters=20):
    """Gauss-Newton for x = (ball centre in {F}, sphere centre in {B}, contact distance).
    F: flange poses (4x4). Returns x, residuals, Jacobian."""
    x = np.concatenate([TCP_NOM, PS_NOM, [L]]) if x0 is None else x0.copy()
    for _ in range(iters):
        Jm, e = [], []
        for T in F:
            r = T[:3, 3] + T[:3, :3] @ x[:3] - x[3:6]
            d = np.linalg.norm(r)
            u = r / d
            e.append(d - x[6])
            Jm.append(np.concatenate([u @ T[:3, :3], -u, [-1.0]]))
        Jm, e = np.array(Jm), np.array(e)
        dx = np.linalg.lstsq(Jm, -e, rcond=None)[0]
        x += dx
        if np.abs(dx).max() < 1e-10:
            break
    return x, e, Jm


def fit_sphere(points, iters=20):
    """Gauss-Newton sphere fit: centre and radius of points (N x 3)."""
    c = points.mean(axis=0)
    x = np.concatenate([c, [np.linalg.norm(points - c, axis=1).mean()]])
    for _ in range(iters):
        r = points - x[:3]
        d = np.linalg.norm(r, axis=1)
        Jm = np.hstack([-r / d[:, None], -np.ones((len(d), 1))])
        dx = np.linalg.lstsq(Jm, -(d - x[3]), rcond=None)[0]
        x += dx
        if np.abs(dx).max() < 1e-12:
            break
    return x, d - x[3]
