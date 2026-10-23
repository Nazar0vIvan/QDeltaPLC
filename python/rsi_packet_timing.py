"""Timing analysis of one RSI run recorded by RoboCrap.

Usage:
    python -I python/rsi_packet_timing.py <rsi_packets_ID.csv> [<rsi_plan_ID.csv>]

Packet capture (rsi_packets_ID.csv): one row per controller request ("rx") and per PC
reply ("tx"): pc_ns, direction, stream_index, write_bytes, write_ns, xml_base64, optionally
followed by entry_ns, sleep_ns, wake_ns, prev_done_ns on reply rows (handler entry, last
event-loop sleep and wake, end of the previous replying handler; format in
resources/files/rsi/RSI_Chamfer_Hole_CONTRACT.md). The script reports late reads, replies the
controller rejected and, with the plan file, how far the rejected corrections moved the
final pose.
"""

import argparse
import base64
import csv
import math
import re

import numpy as np

CYCLE_MS = 4                # IPOC step of one RSI cycle.
LATE_READ_MS = 2.0          # A request read later than this after its reference time.
SAME_READ_MS = 0.3          # Requests read closer together were read in one go.
WOKE_TO_ENTRY_MS = 0.5      # Handler start after wake-up still counts as caused by the wake.
OWN_WORK_MS = 1.0           # Previous handler finishing this long after its reply.
SETTLE_CYCLES = 50          # Zero corrections the app appends after the motion.

IPOC_RE = re.compile(rb"<IPOC>(\d+)</IPOC>")
DELAY_RE = re.compile(rb'<Delay D="(\d+)"')
RIST_RE = re.compile(rb'<RIst X="([^"]*)" Y="([^"]*)" Z="([^"]*)" A="([^"]*)" B="([^"]*)" C="([^"]*)"')
STOP_RE = re.compile(rb"<Stop>1</Stop>")
RESULT_RE = re.compile(rb"<Result>1</Result>")


def read_packets(path):
    """Requests and replies in file order, each as a dict."""
    packets = []
    with open(path, newline="", encoding="utf-8") as file:
        rows = csv.reader(line for line in file if not line.startswith("#"))
        for row in rows:
            if not row or row[0] == "pc_ns":
                continue
            xml = base64.b64decode(row[5])
            delay = DELAY_RE.search(xml)
            rist = RIST_RE.search(xml)
            packet = {
                "ns": int(row[0]),
                "rx": row[1] == "rx",
                "index": int(row[2]),
                "ipoc": int(IPOC_RE.search(xml).group(1)),
                "delay": int(delay.group(1)) if delay else None,
                "rist": tuple(float(value) for value in rist.groups()) if rist else None,
                "control": bool(STOP_RE.search(xml) or RESULT_RE.search(xml)),
                "times": [int(value) for value in row[6:10]] if len(row) >= 10 else None,
            }
            packets.append(packet)
    return packets


def reference_fit(requests):
    """Line pc_ns = a * IPOC + b through the earliest-read 20 % of requests."""
    ipoc = np.array([request["ipoc"] for request in requests], float)
    ns = np.array([request["ns"] for request in requests], float)
    selected = np.ones(len(ns), bool)
    for _ in range(6):
        a, b = np.polyfit(ipoc[selected], ns[selected], 1)
        residual = ns - (a * ipoc + b)
        selected = residual <= np.percentile(residual, 20)
    return a, b


def late_reads(requests, lateness):
    """Groups of requests read in one go whose first request was read late."""
    groups = []
    first = 0
    while first < len(requests):
        last = first
        while (last + 1 < len(requests)
               and requests[last + 1]["ns"] - requests[last]["ns"] < SAME_READ_MS * 1e6):
            last += 1
        if lateness[requests[first]["ipoc"]] > LATE_READ_MS:
            groups.append((first, last - first + 1))
        first = last + 1
    return groups


def rotation(a, b, c):
    """KUKA A/B/C in degrees: Rz(A) Ry(B) Rx(C)."""
    a, b, c = (math.radians(value) for value in (a, b, c))
    rz = np.array([[math.cos(a), -math.sin(a), 0], [math.sin(a), math.cos(a), 0], [0, 0, 1]])
    ry = np.array([[math.cos(b), 0, math.sin(b)], [0, 1, 0], [-math.sin(b), 0, math.cos(b)]])
    rx = np.array([[1, 0, 0], [0, math.cos(c), -math.sin(c)], [0, math.sin(c), math.cos(c)]])
    return rz @ ry @ rx


def angle_between(first, second):
    """Shortest rotation angle in degrees between two rotation matrices."""
    cosine = (np.trace(first.T @ second) - 1.0) / 2.0
    return math.degrees(math.acos(max(-1.0, min(1.0, cosine))))


def read_plan(path):
    """Start pose (row 0 target) and the motion corrections, one per stream index."""
    with open(path, newline="", encoding="utf-8") as file:
        rows = [row for row in csv.reader(line for line in file if not line.startswith("#"))
                if row and row[0] != "time_s"]
    values = [[float(value) for value in row] for row in rows]
    return values[0][1:7], [row[13:19] for row in values[1:]]


def replay(start, corrections, skipped):
    """Position and rotation after composing the corrections as RSI POSCORR (WORLD, relative)."""
    position = np.array(start[:3])
    orientation = rotation(*start[3:])
    for index, correction in enumerate(corrections):
        if index in skipped:
            continue
        position = position + np.array(correction[:3])
        orientation = rotation(*correction[3:]) @ orientation
    return position, orientation


def classify(reply, previous_reply, reference_ns):
    """Cause of a late read from the reply's thread timestamps (Milestone 2 columns)."""
    if previous_reply is None:
        return "first reply, no previous handler to compare"
    entry_ns, sleep_ns, wake_ns, prev_done_ns = reply["times"]
    slept = sleep_ns > prev_done_ns
    if prev_done_ns - previous_reply["ns"] > OWN_WORK_MS * 1e6:
        return "own work"
    # Before the next wait (RsiDevice::serve runs processEvents there).
    if sleep_ns - prev_done_ns > OWN_WORK_MS * 1e6:
        return "busy elsewhere (%.3f ms before waiting)" % ((sleep_ns - prev_done_ns) / 1e6)
    if slept and entry_ns - wake_ns <= WOKE_TO_ENTRY_MS * 1e6:
        return "woke late (%.3f ms after reference)" % ((wake_ns - reference_ns) / 1e6)
    return "busy elsewhere"


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("packets")
    parser.add_argument("plan", nargs="?")
    args = parser.parse_args()

    packets = read_packets(args.packets)
    requests = [packet for packet in packets if packet["rx"]]
    replies = [packet for packet in packets if not packet["rx"]]
    print("requests %d, replies %d" % (len(requests), len(replies)))

    a, b = reference_fit(requests)
    reference = {request["ipoc"]: a * request["ipoc"] + b for request in requests}
    lateness = {request["ipoc"]: (request["ns"] - reference[request["ipoc"]]) / 1e6
                for request in requests}
    print("reference fit: %.3f PC ns per IPOC ms; typical read lateness %.3f ms"
          % (a, float(np.median(list(lateness.values())))))

    by_ipoc = {request["ipoc"]: request for request in requests}
    has_delay = any(request["delay"] is not None for request in requests)
    rejected = []
    if has_delay:
        for reply in replies:
            following = by_ipoc.get(reply["ipoc"] + CYCLE_MS)
            if following is not None and following["delay"]:
                rejected.append(reply)
        print("rejected replies %d: stream indices %s"
              % (len(rejected), " ".join(str(reply["index"]) for reply in rejected) or "none"))
        for reply in rejected:
            print("  index %d: request read %.3f ms late, reply sent %.3f ms after reference%s"
                  % (reply["index"], lateness[reply["ipoc"]],
                     (reply["ns"] - reference[reply["ipoc"]]) / 1e6,
                     ", stop/result reply" if reply["control"] else ""))
    else:
        print("rejected replies: unknown (the capture has no Delay field)")

    groups = late_reads(requests, lateness)
    print("late reads (> %.0f ms) %d" % (LATE_READ_MS, len(groups)))
    reply_after = {}
    previous = None
    for packet in packets:
        if not packet["rx"]:
            reply_after[packet["ipoc"]] = (packet, previous)
            previous = packet
    for first, count in groups:
        request = requests[first]
        line = "  t=%8.3f s: %d request(s), first read %.3f ms late" % (
            (request["ns"] - requests[0]["ns"]) / 1e9, count, lateness[request["ipoc"]])
        newest = requests[first + count - 1]["ipoc"]
        reply, previous_reply = reply_after.get(newest, (None, None))
        if reply is not None and reply["times"] is not None:
            line += "; " + classify(reply, previous_reply, reference[request["ipoc"]])
        print(line)

    if args.plan:
        start, corrections = read_plan(args.plan)
        final = next(packet["rist"] for packet in reversed(requests) if packet["rist"])
        measured = rotation(*final[3:])
        skipped = {reply["index"] for reply in rejected if reply["index"] < len(corrections)}
        print("plan: %d motion corrections + %d settle zeros; %d rejected motion corrections"
              % (len(corrections), SETTLE_CYCLES, len(skipped)))
        for name, omitted in (("all applied", set()), ("rejected omitted", skipped)):
            position, orientation = replay(start, corrections, omitted)
            print("final difference to RIst, %s: %.3f deg, %.3f mm"
                  % (name, angle_between(orientation, measured),
                     float(np.linalg.norm(position - np.array(final[:3])))))


if __name__ == "__main__":
    main()
