import numpy as np
import json
import matplotlib.pyplot as plt
from scipy.signal import butter, sosfiltfilt

files = [
    #"record -0.2 -w-cut -A=0 -1.json",
    #"record -0.2 -w-cut -A=0 -2.json",
    #"record -0.2 -w-cut -A=0 -3.json",

    #"record -0.5 -wo-cut -A=0.json",

    #"record -0.5 -wo-cut -1.json",
    #"record -1.0 -wo-cut -1.json",
    #"record -1.5 -wo-cut -1.json",
    "record -0.0 -wo-cut -A=0.json"
]

scale = 1_000_000.0

label_fs = 18
tick_fs = 16
legend_fs = 18
title_fs = 18


def load_record(filename):
    with open(filename, "r", encoding="utf-8") as f:
        data = json.load(f)

    samples = data["samples"]

    t = [s["timestamp"] for s in samples]

    Fx = [s["Fx"] / scale for s in samples]
    Fy = [s["Fy"] / scale for s in samples]
    Fz = [s["Fz"] / scale for s in samples]

    Tx = [s["Tx"] / scale for s in samples]
    Ty = [s["Ty"] / scale for s in samples]
    Tz = [s["Tz"] / scale for s in samples]

    return t, Fx, Fy, Fz, Tx, Ty, Tz

def extract_top_fz_harmonics(
    json_path: str,
    top_n: int = 5,
    min_freq_hz: float = 0.5,
    max_freq_hz: float = None,
    plot_original: bool = True,
):
    """
    Extract dominant frequency components from Fz signal and plot them.

    Returns:
        harmonics: list[dict] with frequency, amplitude, phase
        t_uniform: uniformly sampled time array
        fz_centered: Fz signal after DC removal
        components: list[np.ndarray] reconstructed harmonic components
    """

    with open(json_path, "r") as f:
        data = json.load(f)

    samples = data["samples"]

    t = np.array([s["timestamp"] for s in samples], dtype=float)
    fz = np.array([s["Fz"] for s in samples], dtype=float)

    # Sort just in case
    order = np.argsort(t)
    t = t[order]
    fz = fz[order]

    # Resample to uniform grid, because FFT assumes constant sampling interval
    dt = np.median(np.diff(t))
    fs = 1.0 / dt

    t_uniform = np.arange(t[0], t[-1], dt)
    fz_uniform = np.interp(t_uniform, t, fz)

    # Remove DC component
    fz_centered = fz_uniform - np.mean(fz_uniform)

    n = len(fz_centered)
    tau = t_uniform - t_uniform[0]

    # Windowing reduces spectral leakage
    window = np.hanning(n)
    xw = fz_centered * window

    fft = np.fft.rfft(xw)
    freqs = np.fft.rfftfreq(n, d=dt)

    # Single-sided amplitude normalization for Hann window
    amplitudes = 2.0 * np.abs(fft) / np.sum(window)
    amplitudes[0] = 0.0  # ignore DC

    if max_freq_hz is None:
        max_freq_hz = fs / 2.0

    valid = (freqs >= min_freq_hz) & (freqs <= max_freq_hz)

    # Keep only local spectral peaks, not just neighboring FFT bins
    peak_mask = np.zeros_like(amplitudes, dtype=bool)
    peak_mask[1:-1] = (
        (amplitudes[1:-1] > amplitudes[:-2]) &
        (amplitudes[1:-1] >= amplitudes[2:])
    )

    candidate_idx = np.where(valid & peak_mask)[0]

    if len(candidate_idx) == 0:
        raise ValueError("No frequency peaks found. Try lowering min_freq_hz.")

    top_idx = candidate_idx[np.argsort(amplitudes[candidate_idx])[-top_n:]][::-1]

    harmonics = []
    components = []

    for idx in top_idx:
        freq = freqs[idx]
        amp = amplitudes[idx]
        phase = np.angle(fft[idx])

        component = amp * np.cos(2.0 * np.pi * freq * tau + phase)

        harmonics.append({
            "frequency_hz": freq,
            "amplitude": amp,
            "phase_rad": phase,
        })
        components.append(component)

    # Plot all extracted harmonics on one chart
    plt.figure(figsize=(12, 6))

    if plot_original:
        plt.plot(t_uniform, fz_centered, linewidth=1.0, alpha=0.35, label="Fz centered")

    for h, component in zip(harmonics, components):
        plt.plot(
            t_uniform,
            component,
            linewidth=1.5,
            label=f"{h['frequency_hz']:.3f} Hz, amp={h['amplitude']:.0f}"
        )

    reconstructed = np.sum(components, axis=0)
    plt.plot(
        t_uniform,
        reconstructed,
        linewidth=2.0,
        linestyle="--",
        label=f"sum of top {len(components)} harmonics"
    )

    plt.xlabel("Time, s")
    plt.ylabel("Fz, raw units")
    plt.title("Dominant Fz harmonics")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.show()

    return harmonics, t_uniform, fz_centered, components

def lowpass_filter_fz(
    json_path,
    cutoff_hz,
    order=4,
    plot=True,
):
    """
    Apply low-pass filter to original Fz signal.

    Args:
        json_path: path to sensor JSON file
        cutoff_hz: cutoff frequency in Hz
        order: Butterworth filter order
        plot: whether to plot original and filtered signal

    Returns:
        t_uniform: uniform time array
        fz_uniform: original Fz resampled to uniform grid
        fz_filtered: low-pass filtered Fz
        fs: sampling frequency
    """

    with open(json_path, "r") as f:
        data = json.load(f)

    samples = data["samples"]

    t = np.array([s["timestamp"] for s in samples], dtype=float)
    fz = np.array([s["Fz"] for s in samples], dtype=float)

    # Sort by timestamp
    order_idx = np.argsort(t)
    t = t[order_idx]
    fz = fz[order_idx]

    # Estimate sampling interval and sampling frequency
    dt = np.median(np.diff(t))
    fs = 1.0 / dt
    nyquist = fs / 2.0

    if cutoff_hz <= 0.0:
        raise ValueError("cutoff_hz must be positive")

    if cutoff_hz >= nyquist:
        raise ValueError(
            "cutoff_hz must be lower than Nyquist frequency: %.3f Hz" % nyquist
        )

    # Resample to uniform timestamps
    t_uniform = np.arange(t[0], t[-1], dt)
    fz_uniform = np.interp(t_uniform, t, fz)

    # Butterworth low-pass filter
    sos = butter(
        N=order,
        Wn=cutoff_hz,
        btype="lowpass",
        fs=fs,
        output="sos",
    )

    # Zero-phase filtering: no time shift
    fz_filtered = sosfiltfilt(sos, fz_uniform)

    if plot:
        plt.figure(figsize=(12, 6))

        plt.plot(
            t_uniform,
            fz_uniform,
            linewidth=1.0,
            alpha=0.35,
            label="Original Fz",
        )

        plt.plot(
            t_uniform,
            fz_filtered,
            linewidth=2.0,
            label="Low-pass Fz, cutoff = %.2f Hz" % cutoff_hz,
        )

        plt.xlabel("Time, s")
        plt.ylabel("Fz, raw units")
        plt.title("Fz low-pass filtering")
        plt.grid(True)
        plt.legend()
        plt.tight_layout()
        plt.show()

    return t_uniform, fz_uniform, fz_filtered, fs

for filename in files:
    t, Fx, Fy, Fz, Tx, Ty, Tz = load_record(filename)

    plt.figure()
    plt.plot(t, Fx, label="Fx")
    plt.plot(t, Fy, label="Fy")
    plt.plot(t, Fz, label="Fz")

    plt.title(filename, fontsize=title_fs)
    plt.ylabel("Силы", fontsize=label_fs)

    plt.xticks(fontsize=tick_fs)
    plt.yticks(fontsize=tick_fs)

    plt.legend(fontsize=legend_fs)
    plt.grid(True)

t, fz_original, fz_filtered, fs = lowpass_filter_fz(
    r"e:\Qt\QtProjects\QDeltaPLC\python\record -0.0 -wo-cut -A=0.json",
    cutoff_hz=8.0,
    order=4,
)