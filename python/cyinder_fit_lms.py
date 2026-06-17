import numpy as np
from numpy.linalg import norm
from scipy.optimize import least_squares

# --- data (mm) ---
P = np.array([
    [919.610168, -58.9764290, 643.898193],
    [925.572510, -45.0691261, 638.017822],
    [920.250427, -38.7094193, 624.221802],
    [932.792847, -42.0696000, 613.405701],
    [926.593689, -53.4439316, 604.491821],
    [931.871155, -73.5837402, 637.811646],
    [926.282532, -78.7271000, 620.813232],
    [920.361816, -68.4017792, 606.092000],
    [933.397217, -69.1570206, 606.341553]
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

np.set_printoptions(precision=6, suppress=False)

def fmt_vec(v):
    return np.array2string(
        v,
        formatter={"float_kind": lambda x: f"{x:.6f}"}
    )

print("Best-fit cylinder:")
print(f"r  = {best['r']:.6f}")
print(f"u  = {fmt_vec(best['u'])}")
print(f"P0 = {fmt_vec(best['P0'])}")
print(f"RMS residual (mm) = {best['rms']:.6f}")

P0_closest_to_origin = best["P0"] - (best["P0"] @ best["u"]) * best["u"]
print(f"P0 (closest to origin on axis) = {fmt_vec(P0_closest_to_origin)}")