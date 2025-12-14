import numpy as np
from numpy.linalg import norm
from scipy.optimize import least_squares

# --- data (mm) ---
P = np.array([
    [868.188110,  -9.15719318, 637.474000],
    [868.188110, -16.10526470, 637.829773],
    [868.188110, -24.09978680, 637.742000],
    [865.547180, -24.09978490, 640.362122],
    [865.547180, -16.18233870, 639.752502],
    [865.547180,  -9.36361504, 639.719666],
    [854.477000,  -9.36361408, 643.285217],
    [854.477000, -16.77864070, 643.171326],
    [854.477000, -23.92431640, 643.083618],
    [838.719666, -24.03811450, 635.182373],
    [838.422607, -16.69407650, 635.403442],
    [838.404785,  -9.22007561, 635.362366],
    [834.555847,  -9.29149532, 622.693298],
    [834.824829, -16.73786160, 622.727295],
    [834.524231, -23.52128030, 622.774536],
    [841.655884, -23.66804890, 608.137207],
    [841.567444, -16.08537860, 608.008606],
    [841.255188,  -9.76375198, 607.704224],
], dtype=np.float64)

def unit(v):
    v = np.asarray(v, dtype=np.float64)
    n = norm(v)
    return v / n if n > 0 else v

def angles_from_u(u):
    u = unit(u)
    phi = np.arctan2(u[1], u[0])                 # azimuth
    theta = np.arcsin(np.clip(u[2], -1.0, 1.0))  # elevation
    return phi, theta

def u_from_angles(phi, theta):
    c = np.cos(theta)
    return np.array([c*np.cos(phi), c*np.sin(phi), np.sin(theta)], dtype=np.float64)

def ortho_basis(u):
    u = unit(u)
    a = np.array([1.0, 0.0, 0.0]) if abs(u[0]) < 0.9 else np.array([0.0, 1.0, 0.0])
    v = unit(np.cross(u, a))
    w = np.cross(u, v)
    return v, w

# Center data for numerical stability; P0 will be returned in world frame.
m = P.mean(axis=0)
Q = P - m

def fit_once(u0):
    u0 = unit(u0)
    phi0, theta0 = angles_from_u(u0)

    # initial radius from distances to axis through origin (in centered coords)
    D0 = Q
    d0 = norm(D0 - (D0 @ u0)[:, None] * u0[None, :], axis=1)
    r0 = float(d0.mean())

    x0 = np.array([phi0, theta0, 0.0, 0.0, r0], dtype=np.float64)
    lb = np.array([-np.pi, -np.pi/2, -np.inf, -np.inf, 0.0], dtype=np.float64)
    ub = np.array([ np.pi,  np.pi/2,  np.inf,  np.inf, np.inf], dtype=np.float64)

    def residuals(x):
        phi, theta, b1, b2, r = x
        u = u_from_angles(phi, theta)
        v, w = ortho_basis(u)
        P0_loc = v*b1 + w*b2                 # axis point constrained to plane ⟂ u
        D = Q - P0_loc[None, :]
        d = norm(D - (D @ u)[:, None]*u[None, :], axis=1)
        return d - r

    res = least_squares(
        residuals, x0, bounds=(lb, ub),
        ftol=1e-14, xtol=1e-14, gtol=1e-14,
        max_nfev=20000
    )

    phi, theta, b1, b2, r = res.x
    u = unit(u_from_angles(phi, theta))
    v, w = ortho_basis(u)
    P0_loc = v*b1 + w*b2
    P0_world = m + P0_loc
    rms = float(np.sqrt(np.mean(res.fun**2)))
    return r, u, P0_world, rms, res.cost

# ---- multi-start initial directions ----
# Always include +/-Y because your data are strongly structured by Y-stations.
starts = [np.array([0.0, 1.0, 0.0]), np.array([0.0, -1.0, 0.0])]

# Add PCA eigenvectors as extra candidates (all 3 axes, both signs)
C = np.cov(Q.T)
eigvals, eigvecs = np.linalg.eigh(C)
for k in range(3):
    starts.append(eigvecs[:, k])
    starts.append(-eigvecs[:, k])

best = None
for u0 in starts:
    r, u, P0, rms, cost = fit_once(u0)
    if best is None or cost < best["cost"]:
        best = {"r": r, "u": u, "P0": P0, "rms": rms, "cost": cost}

np.set_printoptions(precision=10, suppress=False)
print("Best-fit cylinder:")
print("r  =", best["r"])
print("u  =", best["u"])
print("P0 =", best["P0"])
print("RMS residual (mm) =", best["rms"])

# Optional: canonicalize P0 to be the closest point on the axis to the origin.
P0_closest_to_origin = best["P0"] - (best["P0"] @ best["u"]) * best["u"]
print("P0 (closest to origin on axis) =", P0_closest_to_origin)