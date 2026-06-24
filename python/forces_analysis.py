import numpy as np
import json
import matplotlib.pyplot as plt
from scipy.signal import butter, sosfiltfilt, welch

scale = 1_000_000.0

label_fs = 18
tick_fs = 16
legend_fs = 18
title_fs = 18

def load_fxfy_record(filename, scale=1_000_000.0):
    with open(filename, "r", encoding="utf-8") as f:
        data = json.load(f)

    samples = data["samples"]

    timestamp = np.array([s["timestamp"] for s in samples], dtype=float)
    Fx = np.array([s["Fx"] / scale for s in samples], dtype=float)
    Fy = np.array([s["Fy"] / scale for s in samples], dtype=float)

    order_idx = np.argsort(timestamp)

    timestamp = timestamp[order_idx]
    Fx = Fx[order_idx]
    Fy = Fy[order_idx]

    time = timestamp - timestamp[0]

    return time, Fx, Fy

def resample_uniform(time, Fx, Fy):
    dt = np.median(np.diff(time))
    fs = 1.0 / dt

    time_u = np.arange(time[0], time[-1], dt)

    Fx_u = np.interp(time_u, time, Fx)
    Fy_u = np.interp(time_u, time, Fy)

    return time_u, Fx_u, Fy_u, fs

def fxfy_psd_spectrum(time_u, Fx_u, Fy_u, fs, nperseg_max=512, plot=True,):
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

def fxfy_fft_spectrum(time_u, Fx_u, Fy_u, fs, remove_mean=True, use_window=True, plot=True,):
    if len(time_u) != len(Fx_u) or len(time_u) != len(Fy_u):
        raise ValueError("time_u, Fx_u and Fy_u must have the same length")
    
    n = len(time_u)

    Fx = Fx_u.copy()
    Fy = Fy_u.copy()

    if remove_mean:
        Fx = Fx - np.mean(Fx)
        Fy = Fy - np.mean(Fy)

    if use_window:
        window = np.hanning(n)
        scale = np.sum(window)
        Fx = Fx * window
        Fy = Fy * window
    else:
        scale = n

    Fx_fft = np.fft.rfft(Fx)
    Fy_fft = np.fft.rfft(Fy)

    hz = np.fft.rfftfreq(n, d=1.0 / fs)

    Fx_amp = 2.0 * np.abs(Fx_fft) / scale
    Fy_amp = 2.0 * np.abs(Fy_fft) / scale

    # DC component should not be doubled
    Fx_amp[0] *= 0.5
    Fy_amp[0] *= 0.5

    # Nyquist component should not be doubled if it exists
    if n % 2 == 0:
        Fx_amp[-1] *= 0.5
        Fy_amp[-1] *= 0.5

    if plot:
        plt.figure(figsize=(14, 7), dpi=100)

        bin_width = hz[1] - hz[0] if len(hz) > 1 else 1.0
        width = bin_width * 0.4

        plt.bar(
            hz - width / 2,
            Fx_amp,
            width=width,
            alpha=0.7,
            label="Fx FFT amplitude",
        )

        plt.bar(
            hz + width / 2,
            Fy_amp,
            width=width,
            alpha=0.7,
            label="Fy FFT amplitude",
        )

        plt.title("Fx/Fy FFT spectrum")
        plt.xlabel("Frequency, Hz")
        plt.ylabel("Amplitude")
        plt.grid(True)
        plt.legend()
        plt.tight_layout()

    return hz, Fx_amp, hz, Fy_amp

def lowpass_filter_channel(
    json_path,
    channel,
    cutoff_hz,
    scale=1_000_000.0,
    order=4,
    plot=True,
):
    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    samples = data["samples"]

    timestamp = np.array([s["timestamp"] for s in samples], dtype=float)
    signal = np.array([s[channel] / scale for s in samples], dtype=float)

    order_idx = np.argsort(timestamp)
    timestamp = timestamp[order_idx]
    signal = signal[order_idx]

    time = timestamp - timestamp[0]

    dt = np.median(np.diff(time))
    fs = 1.0 / dt
    nyquist = fs / 2.0

    if cutoff_hz <= 0.0:
        raise ValueError("cutoff_hz must be positive")

    if cutoff_hz >= nyquist:
        raise ValueError(
            "cutoff_hz must be lower than Nyquist frequency: %.3f Hz" % nyquist
        )

    time_uniform = np.arange(time[0], time[-1], dt)
    signal_uniform = np.interp(time_uniform, time, signal)

    sos = butter(
        N=order,
        Wn=cutoff_hz,
        btype="lowpass",
        fs=fs,
        output="sos",
    )

    signal_filtered = sosfiltfilt(sos, signal_uniform)

    if plot:
        plt.figure(figsize=(12, 6), dpi=100)

        plt.plot(
            time_uniform,
            signal_uniform,
            linewidth=1.0,
            alpha=0.35,
            label="Original " + channel,
        )

        plt.plot(
            time_uniform,
            signal_filtered,
            linewidth=2.0,
            label="Low-pass %s, cutoff = %.2f Hz" % (channel, cutoff_hz),
        )

        plt.xlabel("Time, s")
        plt.ylabel(channel)
        plt.title("%s low-pass filtering" % channel)
        plt.grid(True)
        plt.legend()
        plt.tight_layout()
        plt.show()

    return time_uniform, signal_uniform, signal_filtered, fs

files = [
    "record -0.2 -w-cut -A=0 -1.json",
    #"record -0.2 -w-cut -A=0 -2.json",
    #"record -0.2 -w-cut -A=0 -3.json",

    #"record -0.5 -wo-cut -A=0.json",

    #"record -0.5 -wo-cut -1.json",
    #"record -1.0 -wo-cut -1.json",
    #"record -1.5 -wo-cut -1.json",
    #"record -0.0 -wo-cut -A=0.json"
]

'''
for filename in files:
    time, Fx, Fy, Fz, Tx, Ty, Tz = load_record(filename)

    plt.figure(figsize=(width_px / dpi, height_px / dpi), dpi=dpi)

    plt.plot(time, Fx, label="Fx")
    plt.plot(time, Fy, label="Fy")
    #plt.plot(time, Fz, label="Fz")

    plt.title(filename, fontsize=title_fs)
    plt.xlabel("Время, с", fontsize=label_fs)
    plt.ylabel("Силы", fontsize=label_fs)

    plt.xticks(fontsize=tick_fs)
    plt.yticks(fontsize=tick_fs)

    plt.legend(fontsize=legend_fs)
    plt.grid(True)
    plt.tight_layout()

plt.show()
'''
'''
def resample_uniform(time, signal):
    dt = np.median(np.diff(time))
    fs = 1.0 / dt

    time_uniform = np.arange(time[0], time[-1], dt)
    signal_uniform = np.interp(time_uniform, time, signal)

    return time_uniform, signal_uniform, fs
'''

for filename in files:
    time, Fx, Fy = load_fxfy_record(filename)
    time_u, Fx_u, Fy_u, fs = resample_uniform(time, Fx, Fy)

    Fx_hz, Fx_amp, Fy_hz, Fy_amp = fxfy_fft_spectrum(
        time_u,
        Fx_u,
        Fy_u,
        fs,
        plot=True,
    )
    
    plt.title("Fx/Fy spectrum: " + filename)

plt.show()