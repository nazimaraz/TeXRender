#!/usr/bin/env python3
"""Compare texrender_render_benchmark output against a loose local baseline."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ROW_RE = re.compile(
    r"^(?P<case>\S+)\s+(?P<metric>cold render|render|draw headless)\s+"
    r"(?P<total>[0-9]+(?:\.[0-9]+)?) ms(?: total,\s+(?P<op>[0-9]+(?:\.[0-9]+)?) ms/op)?$"
)


def parse_output(text: str) -> dict[str, dict[str, float]]:
    metrics: dict[str, dict[str, float]] = {}
    for line in text.splitlines():
        match = ROW_RE.match(line.strip())
        if not match:
            continue
        case = match.group("case")
        metric = match.group("metric")
        value = float(match.group("op") or match.group("total"))
        metrics.setdefault(case, {})[metric] = value
    return metrics


def read_input(path: Path | None, command: list[str] | None) -> str:
    if command:
        return subprocess.check_output(command, text=True)
    if path:
        return path.read_text(encoding="utf-8")
    return sys.stdin.read()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path, default=ROOT / "docs/performance-baseline.json")
    parser.add_argument("--input", type=Path, help="benchmark output file; stdin is used when omitted")
    parser.add_argument("--command", nargs=argparse.REMAINDER, help="run a benchmark command instead of reading input")
    parser.add_argument("--max-ratio", type=float, help="override baseline threshold ratio")
    parser.add_argument("--warn-only", action="store_true", help="print regressions but exit successfully")
    args = parser.parse_args()

    baseline = json.loads(args.baseline.read_text(encoding="utf-8"))
    threshold = args.max_ratio or float(baseline.get("threshold_ratio", 1.4))
    observed = parse_output(read_input(args.input, args.command))

    failures: list[str] = []
    missing: list[str] = []
    for case, metrics in baseline["metrics"].items():
        for metric, expected in metrics.items():
            try:
                actual = observed[case][metric]
            except KeyError:
                missing.append(f"{case}/{metric}")
                continue
            ratio = actual / float(expected)
            if ratio > threshold:
                failures.append(f"{case}/{metric}: {actual:.3f} ms/op is {ratio:.2f}x baseline {expected:.3f} ms/op")

    if missing:
        failures.append("missing benchmark metrics: " + ", ".join(missing))

    if failures:
        for failure in failures:
            print(failure, file=sys.stderr)
        return 0 if args.warn_only else 1

    print(f"benchmark baseline check passed ({sum(len(v) for v in observed.values())} metrics, threshold {threshold:.2f}x)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
