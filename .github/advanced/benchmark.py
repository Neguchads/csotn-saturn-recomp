"""Informational microbenchmarks (not CI regression thresholds)."""
import json
import os
import platform
from pathlib import Path
from statistics import median
from time import perf_counter_ns

def measure(label, work, repeats=200):
    work()
    observations = []
    for _ in range(5):
        started = perf_counter_ns()
        for unused in range(repeats):
            work()
        observations.append((perf_counter_ns() - started) / repeats / 1_000_000)
    return {"name": label, "median_ms_per_operation": round(median(observations), 6),
            "samples_ms": [round(t, 6) for t in observations]}

def emit(metrics):
    payload = {"schema": "stage3-benchmarks-v1", "repository": os.getenv("GITHUB_REPOSITORY", "local"),
               "sha": os.getenv("GITHUB_SHA", ""), "python": platform.python_version(),
               "os": platform.system(), "metrics": metrics,
               "note": "Observational only: runner noise prevents comparing isolated runs as a strict CI gate."}
    Path("benchmark-results.json").write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(payload, indent=2))
    summary = os.getenv("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as f:
            f.write("### Stage 3 benchmark (informational)\n\n")
            f.write("| Test | median ms/op |\n| --- | ---: |\n")
            for metric in metrics:
                f.write(f"| {metric['name']} | {metric['median_ms_per_operation']} |\n")


from scripts import check_repository as hygiene
SYNTHETIC_TEXT = b"synthetic source file without game media\n" * 12
PATHS = [f"docs/section_{i}.md" for i in range(100)]

if __name__ == "__main__":
    metrics = [
        measure("publication_allowed_100_paths", lambda: [hygiene.allowed(p) for p in PATHS], 100),
        measure("publication_inspect_text", lambda: hygiene.inspect("docs/synthetic.md", SYNTHETIC_TEXT), 1000),
    ]
    emit(metrics)
