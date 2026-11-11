"""Touch file of one RSI_Probe_Calibration session.

Usage:
    python -I python/rsi_touches.py <records directory> <id>

Reads the packet captures rsi_packets_<id>.csv and rsi_packets_<id>_partN.csv that RsiDevice saved
during the session (format in resources/files/rsi/Chamfer/RSI_Chamfer_Hole_CONTRACT.md), splits the
received controller packets into searches and writes rsi_touches_<id>.csv into the same directory
(format in resources/files/rsi/Calibration/RSI_Probe_Calibration_CONTRACT.md).

A search is a maximal sequence of received packets with Start 1 and the same numeric TouchIndex
greater than 0. Its row comes from the first packet with Touch 1: the IPOC, the cycle count
(IPOC - first IPOC of the search) / 4, AIPos A1-A6 and the AIPos of the preceding packet of the
search. Searches without Touch 1 are listed and the file is still written. Exit status 1 when a
capture dropped rows, a packet cannot be decoded, Touch is not 0/1 during a search or a touch number
was searched more than once.
"""

import argparse
import base64
import collections
import csv
import glob
import os
import re
import sys
import xml.etree.ElementTree as ElementTree

CYCLE_MS = 4  # IPOC step of one RSI cycle (milliseconds of controller time).
AXES = ("A1", "A2", "A3", "A4", "A5", "A6")


def capture_files(directory, plan_id):
    """The session's captures in save order: the first file, then _part2, _part3, ..."""
    first = os.path.join(directory, "rsi_packets_%s.csv" % plan_id)
    pattern = os.path.join(glob.escape(directory), glob.escape("rsi_packets_%s_part" % plan_id) + "*.csv")
    parts = []
    for path in glob.glob(pattern):
        match = re.fullmatch(r"rsi_packets_.+_part(\d+)\.csv", os.path.basename(path))
        if match:
            parts.append((int(match.group(1)), path))
    files = [first] if os.path.exists(first) else []
    return files + [path for _, path in sorted(parts)]


def decode(xml_base64):
    """Start, TouchIndex, Touch, IPOC and AIPos of one <Rob> packet; None if it cannot be decoded."""
    try:
        root = ElementTree.fromstring(base64.b64decode(xml_base64, validate=True))
    except (ValueError, ElementTree.ParseError):
        return None
    aipos = root.find("AIPos")
    start = root.findtext("Start")
    ipoc = root.findtext("IPOC")
    if root.tag != "Rob" or aipos is None or start is None or ipoc is None or not ipoc.strip().isdigit():
        return None
    angles = [aipos.get(axis) for axis in AXES]
    if None in angles:
        return None
    return {
        "start": start.strip() == "1",
        "touch_index": (root.findtext("TouchIndex") or "").strip(),
        "touch": (root.findtext("Touch") or "").strip(),
        "ipoc": int(ipoc),
        "angles": angles,  # Text as sent, six decimals with Precision 6.
    }


def read_capture(path):
    """Received packets of one capture file, its dropped-row count and its undecodable packets."""
    packets, dropped, invalid = [], 0, 0
    with open(path, newline="") as file:
        for row in csv.reader(file):
            if not row:
                continue
            if row[0] == "# dropped_rows" and len(row) > 1:
                dropped += int(row[1])
            if row[0].startswith("#") or len(row) < 6 or row[1] != "rx":
                continue
            packet = decode(row[5])
            if packet is None:
                invalid += 1
            else:
                packets.append(packet)
    return packets, dropped, invalid


def searches(packets):
    """(touch number, packets) of every maximal run with Start 1 and one TouchIndex above 0."""
    runs, current, number = [], None, None
    for packet in packets:
        index = packet["touch_index"]
        key = int(index) if packet["start"] and index.isdigit() and int(index) > 0 else None
        if key is None:
            current = None
            continue
        if current is None or key != number:
            current, number = [], key
            runs.append((key, current))
        current.append(packet)
    return runs


def touch_row(number, run):
    """The run's touch row, or None when no packet of the run has Touch 1."""
    for position, packet in enumerate(run):
        if packet["touch"] == "1":
            previous = run[position - 1]["angles"] if position > 0 else [""] * len(AXES)
            cycle = (packet["ipoc"] - run[0]["ipoc"]) // CYCLE_MS
            return [number, packet["ipoc"], cycle] + packet["angles"] + previous
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("records", help="records directory beside robocrap.exe")
    parser.add_argument("id", help="plan id of the calibration RSI Sampling, for example 20261111_131825_176")
    arguments = parser.parse_args()

    files = capture_files(arguments.records, arguments.id)
    if not files:
        print("No rsi_packets_%s*.csv in %s." % (arguments.id, arguments.records), file=sys.stderr)
        return 2
    packets, problems = [], []
    for path in files:
        file_packets, dropped, invalid = read_capture(path)
        packets += file_packets
        if dropped:
            problems.append("%s: %d dropped rows" % (os.path.basename(path), dropped))
        if invalid:
            problems.append("%s: %d undecodable packets" % (os.path.basename(path), invalid))

    runs = searches(packets)
    rows, missed = [], []
    for number, run in runs:
        unknown = sorted({packet["touch"] for packet in run} - {"0", "1"})
        if unknown:
            problems.append("touch %d: Touch values %s (is DIGIN1 connected to ETHERNET1.In4?)" % (number, unknown))
        row = touch_row(number, run)
        if row is None:
            missed.append(number)
        else:
            rows.append(row)
    repeated = sorted(number for number, count in collections.Counter(number for number, _ in runs).items() if count > 1)
    if repeated:
        problems.append("touch numbers searched more than once: %s" % repeated)

    output = os.path.join(arguments.records, "rsi_touches_%s.csv" % arguments.id)
    with open(output, "w", newline="") as file:
        file.write("# plan_id,%s\n" % arguments.id)
        file.write("# cycle: (IPOC - first IPOC of the run) / 4; AIPos deg; prev_*: preceding packet\n")
        writer = csv.writer(file, lineterminator="\n")
        writer.writerow(["touch_index", "ipoc", "cycle"] + list(AXES) + ["prev_" + axis for axis in AXES])
        writer.writerows(rows)

    print("%d capture files, %d received packets, %d searches, %d touches written to %s"
          % (len(files), len(packets), len(runs), len(rows), output))
    if missed:
        print("searches without Touch 1: %s" % missed)
    for problem in problems:
        print("ERROR: " + problem, file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
