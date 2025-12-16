import json
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D  # noqa: F401

path = "99.01.25.242.json"

with open(path, "r", encoding="utf-8") as f:
    root = json.load(f)

# Supports either: root = [ {cx: [...] , ...}, ... ]  OR  root = {"profiles": [ ... ]}
profiles = root["profiles"] if isinstance(root, dict) and "profiles" in root else root
if not isinstance(profiles, list):
    raise TypeError("Expected JSON root to be a list of profiles or a dict with key 'profiles'.")

fig = plt.figure()
ax = fig.add_subplot(111, projection="3d")

all_pts = []
for i, prof in enumerate(profiles):
    cx = np.asarray(prof.get("cx", []), dtype=float)
    if cx.ndim != 2 or cx.shape[1] != 3 or cx.shape[0] == 0:
        continue

    ax.plot(cx[:, 0], cx[:, 1], cx[:, 2], marker=".", linewidth=0.8, markersize=2)
    all_pts.append(cx)

# Optional: nicer equal-ish scaling in 3D
if all_pts:
    pts = np.vstack(all_pts)
    mins = pts.min(axis=0)
    maxs = pts.max(axis=0)
    ax.set_xlim(mins[0], maxs[0])
    ax.set_ylim(mins[1], maxs[1])
    ax.set_zlim(mins[2], maxs[2])
    ax.set_box_aspect((maxs - mins))  # matplotlib >= 3.3

ax.set_xlabel("X")
ax.set_ylabel("Y")
ax.set_zlabel("Z")
ax.set_title("CX points for each profile (3D)")
plt.show()
