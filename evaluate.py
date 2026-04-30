#!/usr/bin/env python3
import os
import platform
import re
import statistics
import subprocess
import sys


def run_cmd(cmd):
    try:
        result = subprocess.run(
            cmd,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        return True, result.stdout
    except subprocess.CalledProcessError as exc:
        return False, exc.stdout


def set_process_affinity(pid, cpu_ids):
    if platform.system() != "Linux" or not hasattr(os, "sched_setaffinity"):
        return False
    try:
        os.sched_setaffinity(pid, set(cpu_ids))
        return True
    except Exception:
        return False


def run_score_once(score_exe, index, total):
    process = subprocess.Popen(
        [score_exe],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if process.poll() is None:
        set_process_affinity(process.pid, [0, 1])

    out, _ = process.communicate()
    if process.returncode != 0:
        print(f"[SCORE_FAILED] run {index}/{total} failed:\n{out}")
        sys.exit(1)

    lines = [line.strip() for line in out.splitlines() if line.strip()]
    if not lines:
        print(f"[SCORE_FAILED] run {index}/{total} produced no output")
        sys.exit(1)

    matches = re.findall(r"time_ms=(\d+(?:\.\d+)?)", lines[-1])
    if not matches:
        print(f"[SCORE_FAILED] could not parse score from: {lines[-1]}")
        sys.exit(1)

    score = float(matches[-1])
    print(f"  Run {index}/{total} | Score (time_ms): {score:.2f}")
    return score


def main():
    iterations = int(os.environ.get("EVAL_RUNS", "15"))

    print("=== Step 1: Build ===")
    success, out = run_cmd(["make", "clean"])
    if not success:
        print("[BUILD_FAILED] clean failed:\n" + out)
        sys.exit(1)

    success, out = run_cmd(["make"])
    if not success:
        print("[BUILD_FAILED] compilation failed:\n" + out)
        sys.exit(1)
    print("Build successful.\n")

    print("=== Step 2: Correctness Tests ===")
    test_exe = os.path.abspath("test")
    success, out = run_cmd([test_exe])
    if not success or "All tests passed (6/6)." not in out:
        print("[TEST_FAILED] correctness tests failed:\n" + out)
        sys.exit(1)
    print(out.strip() + "\n")

    print("=== Step 3: Benchmark ===")
    score_exe = os.path.abspath("score")
    scores = [run_score_once(score_exe, i + 1, iterations) for i in range(iterations)]

    median_score = statistics.median(scores)
    min_score = min(scores)
    mean_score = statistics.mean(scores)

    print("\n=== Evaluation Complete ===")
    print(f"MIN_SCORE: {min_score:.2f}")
    print(f"MEDIAN_SCORE: {median_score:.2f}")
    print(f"MEAN_SCORE: {mean_score:.2f}")
    print("NOTE: score is elapsed time in milliseconds. LOWER IS BETTER.")

    with open("score_output.txt", "w", encoding="utf-8") as f:
        f.write(f"MEDIAN_SCORE: {median_score:.2f}\n")
        f.write(f"MIN_SCORE: {min_score:.2f}\n")
        f.write(f"MEAN_SCORE: {mean_score:.2f}\n")


if __name__ == "__main__":
    main()
