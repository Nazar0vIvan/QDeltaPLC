import numpy as np
import json
import matplotlib.pyplot as plt


#  
with open("record.json", "r", encoding="utf-8") as f:
    data = json.load(f)

samples = data["samples"]
t  = [s["timestamp"] for s in samples]

# Scale (divide by 1_000_000)
scale = 1_000_000.0
Fx = [s["Fx"] / scale for s in samples]
Fy = [s["Fy"] / scale for s in samples]
Fz = [s["Fz"] / scale for s in samples]
Tx = [s["Tx"] / scale for s in samples]
Ty = [s["Ty"] / scale for s in samples]
Tz = [s["Tz"] / scale for s in samples]

# Plot forces
label_fs = 18
tick_fs  = 16
legend_fs = 18

plt.figure()
plt.plot(t, Fx, label="Fx")
plt.plot(t, Fy, label="Fy")
plt.plot(t, Fz, label="Fz")

plt.ylabel("Силы", fontsize=label_fs)

plt.xticks(fontsize=tick_fs)
plt.yticks(fontsize=tick_fs)

plt.legend(fontsize=legend_fs)
plt.grid(True)
plt.show()

'''
# Plot torques
plt.figure()
plt.plot(t, Tx, label="Tx")
plt.plot(t, Ty, label="Ty")
plt.plot(t, Tz, label="Tz")
plt.xlabel("t, s")
plt.ylabel("Torque (scaled)")
plt.title("Torques vs time")
plt.legend()
plt.grid(True)
'''
plt.show()



