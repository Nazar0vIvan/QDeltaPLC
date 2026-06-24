import numpy as np
import json
import matplotlib.pyplot as plt
from scipy.signal import butter, sosfiltfilt, welch
from dataclasses import dataclass

label_fs = 18
tick_fs = 16
legend_fs = 18
title_fs = 18

width_px = 1000
height_px = 600
dpi = 100

scale = 1_000_000.0

@dataclass
class ChartInfo:
    xlabel: str
    ylabel: str
    filename: str
    name: str
    color: str

def load_forces_record(filename):
    with open(filename, "r", encoding="utf-8") as f:
        data = json.load(f)

    samples = data["samples"]

    timestamp = np.array([s["timestamp"] for s in samples], dtype=float)

    Fx = np.array([s["Fx"] / scale for s in samples], dtype=float)
    Fy = np.array([s["Fy"] / scale for s in samples], dtype=float)
    Fz = np.array([s["Fz"] / scale for s in samples], dtype=float)

    order_idx = np.argsort(timestamp)

    timestamp = timestamp[order_idx]
    Fx = Fx[order_idx]
    Fy = Fy[order_idx]
    Fz = Fz[order_idx]

    time = timestamp - timestamp[0]
    
    return time, Fx, Fy, Fz

def resample_uniform(time, signal):
    dt = np.median(np.diff(time))
    fs = 1.0 / dt # how many sensor samples are recorded per second

    time_u = np.arange(time[0], time[-1], dt)
    signal_u = np.interp(time_u, time, signal)

    return time_u, signal_u, fs

def psd_spectrum(time_u, Fx_u, Fy_u, fs, nperseg_max=512, plot=True,):
    Fx_centered = Fx_u - np.mean(Fx_u)
    Fy_centered = Fy_u - np.mean(Fy_u)

    nperseg = min(nperseg_max, len(time_u))

    Fx_hz, Fx_psd = welch(
        Fx_centered,
        fs=fs,
        window="hann",
        nperseg=nperseg,
        noverlap=nperseg // 2,
    )

    Fy_hz, Fy_psd = welch(
        Fy_centered,
        fs=fs,
        window="hann",
        nperseg=nperseg,
        noverlap=nperseg // 2,
    )

    if plot:
        plt.figure(figsize=(14, 7), dpi=100)

        bin_width = Fx_hz[1] - Fx_hz[0] if len(Fx_hz) > 1 else 1.0
        width = bin_width * 0.4

        plt.bar(
            Fx_hz - width / 2,
            Fx_psd,
            width=width,
            alpha=0.7,
            label="Fx PSD",
        )

        plt.bar(
            Fy_hz + width / 2,
            Fy_psd,
            width=width,
            alpha=0.7,
            label="Fy PSD",
        )

        plt.yscale("log")
        plt.title("Fx/Fy spectrum")
        plt.xlabel("Frequency, Hz")
        plt.ylabel("PSD")
        plt.grid(True, which="both")
        plt.legend()
        plt.tight_layout()

    return Fx_hz, Fx_psd, Fy_hz, Fy_psd

def fft_spectrum(time_u, signal_u, fs, remove_mean=True, use_hann_window=True):
    if len(time_u) != len(signal_u):
        raise ValueError("time_u and signal_u must have the same length")

    n = len(time_u)

    signal = signal_u.copy()

    if remove_mean:
        signal = signal - np.mean(signal)

    if use_hann_window:
        window = np.hanning(n)
        normalization = np.sum(window)
        signal = signal * window
    else:
        normalization = n

    signal_fft = np.fft.rfft(signal)

    hz = np.fft.rfftfreq(n, d=1.0 / fs)

    amp = 2.0 * np.abs(signal_fft) / normalization

    # DC component should not be doubled
    amp[0] *= 0.5

    # Nyquist component should not be doubled if it exists
    if n % 2 == 0:
        amp[-1] *= 0.5

    return hz, amp

def plot_line_chart(x, y, info):
    plt.figure(figsize=(width_px / dpi, height_px / dpi), dpi=dpi)

    plt.plot(
        x,
        y,
        label=info.name,
        color=info.color,
    )

    plt.title(f"{info.filename}: {info.name}", fontsize=title_fs)
    plt.xlabel(info.xlabel, fontsize=label_fs)
    plt.ylabel(info.ylabel, fontsize=label_fs)

    plt.xticks(fontsize=tick_fs)
    plt.yticks(fontsize=tick_fs)

    plt.legend(fontsize=legend_fs, loc="upper right")
    plt.grid(True)
    plt.tight_layout()

def plot_bar_chart(x, y, info):
    plt.figure(figsize=(width_px / dpi, height_px / dpi), dpi=dpi)

    bin_width = x[1] - x[0] if len(x) > 1 else 1.0
    width = bin_width * 0.8

    plt.bar(
        x,
        y,
        width=width,
        label=info.name,
        color=info.color,
        alpha=0.7,
    )

    plt.title(f"{info.filename}: {info.name}", fontsize=title_fs)
    plt.xlabel(info.xlabel, fontsize=label_fs)
    plt.ylabel(info.ylabel, fontsize=label_fs)

    plt.xticks(fontsize=tick_fs)
    plt.yticks(fontsize=tick_fs)

    plt.legend(fontsize=legend_fs, loc="upper right")
    plt.grid(True)
    plt.tight_layout()

def butter_filter(time, signal, cutoff_hz, order=4):
    time_u, signal_u, fs = resample_uniform(time, signal)

    nyquist = fs / 2.0

    if cutoff_hz <= 0.0:
        raise ValueError("cutoff_hz must be positive")

    if cutoff_hz >= nyquist:
        raise ValueError(
            f"cutoff_hz must be lower than Nyquist frequency: {nyquist:.3f} Hz"
        )

    sos = butter(
        N=order,
        Wn=cutoff_hz,
        btype="lowpass",
        fs=fs,
        output="sos",
    )

    signal_filtered = sosfiltfilt(sos, signal_u)

    return time_u, signal_filtered

### START

files = [
    "record -0.2 -w-cut -A=0 -1.json",
    #"record -0.2 -w-cut -A=0 -2.json",
    #"record -0.2 -w-cut -A=0 -3.json",

    #"record -0.5 -wo-cut -A=0.json",

    #"record -0.5 -wo-cut -1.json",
    #"record -1.0 -wo-cut -1.json",
    #"record -1.5 -wo-cut -1.json",
    #"record -0.0 -wo-cut -A=0.json",
    "record_fx_8_15_changed_mean_minus_10N_same_vibrations_v2.json"
]

for filename in files:
    time, Fx, Fy, Fz = load_forces_record(filename)

    plot_line_chart(time, Fx,
        ChartInfo(
            xlabel="Time, s",
            ylabel="Force, N",
            filename=filename,
            name="Fx",
            color="red",
        ),
    )

plt.show()
'''
time_u, Fx_u, fs = resample_uniform(time, Fx)
hz, Fx_amp = fft_spectrum(time_u, Fx_u, fs)

plot_bar_chart(
    hz,
    Fx_amp,
    ChartInfo(
        xlabel="Frequency, Hz",
        ylabel="Amplitude",
        filename=filename,
        name="Fx FFT amplitude",
        color="red",
    ),
)

time_u, Fy_u, fs = resample_uniform(time, Fy)
hz, Fy_amp = fft_spectrum(time_u, Fy_u, fs)

plot_bar_chart(
    hz,
    Fy_amp,
    ChartInfo(
        xlabel="Frequency, Hz",
        ylabel="Amplitude",
        filename=filename,
        name="Fy FFT amplitude",
        color="green",
    ),
)

time_u, Fz_u, fs = resample_uniform(time, Fz)
hz, Fz_amp = fft_spectrum(time_u, Fz_u, fs)

plot_bar_chart(
    hz,
    Fz_amp,
    ChartInfo(
        xlabel="Frequency, Hz",
        ylabel="Amplitude",
        filename=filename,
        name="Fz FFT amplitude",
        color="blue",
    ),
)

time_f, Fx_filtered = butter_filter(
    time,
    Fx,
    cutoff_hz=2.0,
    order=4,
)

plot_line_chart(
    time_f,
    Fx_filtered,
    ChartInfo(
        xlabel="Time, s",
        ylabel="Force, N",
        filename=filename,
        name="Fx, Butterworth 5 Hz",
        color="red",
    ),
)

time_f, Fy_filtered = butter_filter(
    time,
    Fy,
    cutoff_hz=2.0,
    order=4,
)

plot_line_chart(
    time_f,
    Fy_filtered,
    ChartInfo(
        xlabel="Time, s",
        ylabel="Force, N",
        filename=filename,
        name="Fy, Butterworth 5 Hz",
        color="green",
    ),
)

time_f, Fz_filtered = butter_filter(
    time,
    Fz,
    cutoff_hz=2.0,
    order=4,
)

plot_line_chart(
    time_f,
    Fz_filtered,
    ChartInfo(
        xlabel="Time, s",
        ylabel="Force, N",
        filename=filename,
        name="Fz, Butterworth 5 Hz",
        color="blue",
    ),
)
'''

