import numpy as np
import matplotlib.pyplot as plt

def set_axes_equal(ax):
    x_limits = ax.get_xlim3d()
    y_limits = ax.get_ylim3d()
    z_limits = ax.get_zlim3d()

    x_range = abs(x_limits[1] - x_limits[0])
    y_range = abs(y_limits[1] - y_limits[0])
    z_range = abs(z_limits[1] - z_limits[0])

    max_range = max(x_range, y_range, z_range) / 2.0

    x_middle = np.mean(x_limits)
    y_middle = np.mean(y_limits)
    z_middle = np.mean(z_limits)

    ax.set_xlim3d(x_middle - max_range, x_middle + max_range)
    ax.set_ylim3d(y_middle - max_range, y_middle + max_range)
    ax.set_zlim3d(z_middle - max_range, z_middle + max_range)

def draw_points_3d():
    points = np.array([
        [919.610168, -58.9764290, 643.898193],
        [925.572510, -45.0691261, 638.017822],
        [920.250427, -38.7094193, 624.221802],
        [932.792847, -42.0696,    613.405701],
        [926.593689, -53.4439316, 604.491821],
        [931.871155, -73.5837402, 637.811646],
        [926.282532, -78.7271,    620.813232],
        [920.361816, -68.4017792, 606.092],
        [933.397217, -69.1570206, 606.341553],
    ])

    special_point = np.array([926.290032, -59.007181, 623.760314])

    labels = [f"XP{i}" for i in range(1, len(points) + 1)]

    fig = plt.figure()
    ax = fig.add_subplot(111, projection="3d")

    xs = points[:, 0]
    ys = points[:, 1]
    zs = points[:, 2]

    ax.scatter(xs, ys, zs, s=50, label="Input points")

    for label, x, y, z in zip(labels, xs, ys, zs):
        ax.text(x, y, z, label)

    # Highlighted point
    ax.scatter(
        special_points[:, 0],
        special_points[:, 1],
        special_points[:, 2],
        s=180,
        marker="*",
        color="red",
        label="Special points"
    )

    for label, x, y, z in zip(
        special_labels,
        special_points[:, 0],
        special_points[:, 1],
        special_points[:, 2],
    ):
        ax.text(x, y, z, f" {label}", color="red")

    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_zlabel("Z")
    ax.set_title("3D Points")

    ax.legend()
    set_axes_equal(ax)

    plt.show()


special_points = np.array([
    [926.290032, -59.007181, 623.760314],
    [926.789, -44.8434, 637.934],
])

special_labels = ["P0", "P1"]

draw_points_3d()