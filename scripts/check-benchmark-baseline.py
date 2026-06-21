#!/usr/bin/env python3
"""Compare texrender_render_benchmark output against a loose local baseline."""

from __future__ import annotations

import argparse
import datetime as dt
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


def normalize_metrics(raw: dict) -> dict[str, dict[str, float]]:
    metrics: dict[str, dict[str, float]] = {}
    for case, values in raw.get("metrics", {}).items():
        case_metrics: dict[str, float] = {}
        for metric, value in values.items():
            if isinstance(value, dict):
                if "ms_per_op" in value:
                    case_metrics[metric] = float(value["ms_per_op"])
                elif "total_ms" in value:
                    iterations = float(value.get("iterations", 1))
                    case_metrics[metric] = float(value["total_ms"]) / iterations
            else:
                case_metrics[metric] = float(value)
        metrics[case] = case_metrics
    return metrics


def parse_text_output(text: str) -> dict[str, dict[str, float]]:
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


def parse_output(text: str) -> dict[str, dict[str, float]]:
    stripped = text.lstrip()
    if stripped.startswith("{"):
        return normalize_metrics(json.loads(stripped))
    return parse_text_output(text)


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
    parser.add_argument("--update-baseline", action="store_true", help="rewrite --baseline metrics from the observed run")
    args = parser.parse_args()

    baseline = json.loads(args.baseline.read_text(encoding="utf-8"))
    threshold = args.max_ratio or float(baseline.get("threshold_ratio", 1.4))
    expected_metrics = normalize_metrics(baseline)
    observed = parse_output(read_input(args.input, args.command))
    if not observed:
        print("no benchmark metrics found in input", file=sys.stderr)
        return 1

    if args.update_baseline:
        baseline["date"] = dt.date.today().isoformat()
        baseline["metrics"] = {
            case: {metric: round(value, 3) for metric, value in metrics.items()} for case, metrics in observed.items()
        }
        args.baseline.write_text(json.dumps(baseline, indent=2, sort_keys=False) + "\n", encoding="utf-8")
        print(f"updated {args.baseline}")
        print("review platform/compiler/build metadata before committing")
        return 0

    failures: list[str] = []
    missing: list[str] = []
    for case, metrics in expected_metrics.items():
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
