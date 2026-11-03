"""KR 10 R1420 kinematics reproduced from src/application/robot/robotdefinition.cpp and
src/kinematics/kr10kinematics.cpp (solveFK), plus a numerical IK for the analysis model."""
import numpy as np

D2R = np.pi / 180.0

# dx, dy, dz, A, B, C (Z-Y-X deg), axis sign, qMin, qMax  (robotdefinition.cpp, Joints)
JOINTS = [
    (0.0, 0.0, 227.0, 0.0, 0.0, 0.0, -1.0, -170.0, 170.0),
    (150.0, -90.4, 223.0, 0.0, 0.0, -90.0, 1.0, -185.0, 65.0),
    (0.0, -610.0, -11.1, 90.0, 0.0, 0.0, 1.0, -137.0, 163.0),
    (-20.0, -357.0, 101.5, 0.0, 0.0, -90.0, 1.0, -185.0, 185.0),
    (0.0, -39.0, -303.0, 0.0, 0.0, 90.0, 1.0, -120.0, 120.0),
    (0.0, -80.0, -39.0, 0.0, 0.0, 90.0, -1.0, -350.0, 350.0),
]
QHOME = np.array([0.0, -90.0, 90.0, 0.0, 0.0, 0.0])
QMIN = np.array([j[7] for j in JOINTS])
QMAX = np.array([j[8] for j in JOINTS])


def rx(d):
    c, s = np.cos(d * D2R), np.sin(d * D2R)
    return np.array([[1, 0, 0], [0, c, -s], [0, s, c]])


def ry(d):
    c, s = np.cos(d * D2R), np.sin(d * D2R)
    return np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]])


def rz(d):
    c, s = np.cos(d * D2R), np.sin(d * D2R)
    return np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]])


def hom(R, p):
    T = np.eye(4)
    T[:3, :3] = R
    T[:3, 3] = p
    return T


def small_rot(v_deg):
    """Rotation by the vector v (deg components), exact Rodrigues."""
    v = np.asarray(v_deg) * D2R
    a = np.linalg.norm(v)
    if a < 1e-15:
        return np.eye(3)
    k = v / a
    K = np.array([[0, -k[2], k[1]], [k[2], 0, -k[0]], [-k[1], k[0], 0]])
    return np.eye(3) + np.sin(a) * K + (1 - np.cos(a)) * K @ K


class Robot:
    def __init__(self):
        self.L = [hom(rz(j[3]) @ ry(j[4]) @ rx(j[5]), np.array(j[:3])) for j in JOINTS]
        self.sign = np.array([j[6] for j in JOINTS])
        self.dq0 = np.zeros(6)
        self.base = np.eye(4)
        self.flange = np.eye(4)

    def copy(self):
        r = Robot()
        r.L = [m.copy() for m in self.L]
        r.dq0 = self.dq0.copy()
        r.base = self.base.copy()
        r.flange = self.flange.copy()
        return r

    def fk(self, q):
        T = self.base.copy()
        for i in range(6):
            T = T @ self.L[i] @ hom(rz(self.sign[i] * (q[i] - QHOME[i] + self.dq0[i])), np.zeros(3))
        return T @ self.flange


def rot_log(R):
    w = np.array([R[2, 1] - R[1, 2], R[0, 2] - R[2, 0], R[1, 0] - R[0, 1]])
    s = np.linalg.norm(w) / 2
    a = np.arctan2(s, (np.trace(R) - 1) / 2)
    if s < 1e-300:
        return np.zeros(3)
    return w * a / (2 * s)


def ik(robot, Tf, seed, iters=60):
    """Damped Newton on flange pose; returns q or None."""
    q = np.array(seed, dtype=float)
    for _ in range(iters):
        T = robot.fk(q)
        ep = Tf[:3, 3] - T[:3, 3]
        er = rot_log(Tf[:3, :3] @ T[:3, :3].T)
        e = np.concatenate([ep, er * 500.0])
        if np.linalg.norm(ep) < 1e-9 and np.linalg.norm(er) < 1e-12:
            return q
        J = np.zeros((6, 6))
        h = 1e-6
        for k in range(6):
            dq = np.zeros(6)
            dq[k] = h
            T2 = robot.fk(q + dq)
            J[:3, k] = (T2[:3, 3] - T[:3, 3]) / h
            J[3:, k] = rot_log(T2[:3, :3] @ T[:3, :3].T) / h * 500.0
        step = np.linalg.solve(J.T @ J + 1e-9 * np.eye(6), J.T @ e)
        n = np.linalg.norm(step)
        if n > 10:
            step *= 10 / n
        q = q + step
    T = robot.fk(q)
    if np.linalg.norm(Tf[:3, 3] - T[:3, 3]) < 1e-7 and np.linalg.norm(rot_log(Tf[:3, :3] @ T[:3, :3].T)) < 1e-10:
        return q
    return None


def ik_ball(robot, ball, R, tcp, seed):
    """Joint angles that put the ball centre (TCP tcp in {F}) at ball with {F}=R."""
    Tf = hom(R, ball - R @ tcp)
    return ik(robot, Tf, seed)
