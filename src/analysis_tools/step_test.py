# /// script
# dependencies = [
#   "requests",
#   "matplotlib",
#   "numpy",
# ]
# ///
"""
Open-loop step test for integrating process (espresso boiler).

Procedure:
  1. Holds the boiler at STEADY_STATE_OUTPUT% for BASELINE_SECS seconds
  2. Steps up to STEP_OUTPUT% and records for STEP_SECS seconds
  3. Fits a ramp to the linear portion of the step response
  4. Calculates Lambda-tuned PI parameters for several closed-loop speeds
  5. Saves data to CSV and shows a plot

Usage:
  uv run step_test.py
  # or: pip install requests matplotlib && python step_test.py
"""

raise NotImplementedError("This script was made for an older version of the API. Update before using!")

import csv
import sys
import time
from datetime import datetime
import logging

import requests
import matplotlib.pyplot as plt
import numpy as np

logger = logging.getLogger(__file__)
logging.basicConfig()
logger.setLevel(logging.DEBUG)

# ─── Configuration ────────────────────────────────────────────────────────────

HOST             = "http://192.168.68.128"  # "http://espresso-pid.local"  # or "http://192.168.x.x"
STEADY_STATE_OUT = 3.3    # % output at which boiler is stable at setpoint
STEP_OUTPUT      = 15     # % output after step
BASELINE_SECS    = 60     # seconds to hold at steady-state before stepping
STEP_SECS        = 90    # seconds to record after step (should cover full ramp)
POLL_HZ          = 1      # samples per second

# Lambda tuning: closed-loop time constants to evaluate (seconds)
# Smaller = faster/more aggressive, larger = slower/more robust
LAMBDAS = [30, 60, 120]

# Fraction of step period to skip at the start before fitting the ramp
# (allows for transport/lag delay before the linear ramp begins)
RAMP_SKIP_FRACTION = 0.15

# ─── Helpers ──────────────────────────────────────────────────────────────────

def get_status():
    r = requests.get(f"{HOST}/api/status", timeout=5)
    r.raise_for_status()
    return r.json()

def set_mode(mode, output=None):
    body = {"mode": mode}
    if output is not None:
        body["output"] = output
    r = requests.post(f"{HOST}/api/mode", json=body, timeout=5)
    r.raise_for_status()
    print(r.json())
    return r.json()

def poll_loop(duration_secs, label):
    """Poll at POLL_HZ for duration_secs. Returns list of (elapsed, temp, output)."""
    records = []
    interval = 1.0 / POLL_HZ
    samples = duration_secs * POLL_HZ
    start = time.monotonic()

    logger.debug(f"{samples=}, {interval=}, {start=}")

    for i in range(samples):
        target = start + i * interval
        now = time.monotonic()
        if target > now:
            time.sleep(target - now)

        try:
            s = get_status()
            elapsed = time.monotonic() - start
            temp = s["temperature"]
            out  = s["output"]
            records.append((elapsed, temp, out))
            print(f"  [{label}] t={elapsed:6.1f}s  T={temp:.1f}°C  output={out:.1f}%")
        except Exception as e:
            print(f"  [WARN] poll failed: {e}")

    return records

# ─── Main ─────────────────────────────────────────────────────────────────────

def main():
    print("=== Espresso boiler step test ===")
    print(f"  Host:           {HOST}")
    print(f"  Steady output:  {STEADY_STATE_OUT}%")
    print(f"  Step output:    {STEP_OUTPUT}%")
    print(f"  Baseline:       {BASELINE_SECS}s  |  Step record: {STEP_SECS}s")
    print()

    # Verify connection
    try:
        status = get_status()
        print(f"Connected. Mode={status['mode']}, T={status['temperature']}°C")
    except Exception as e:
        print(f"Cannot reach {HOST}: {e}")
        sys.exit(1)

    # Phase 1 — hold at steady state
    print(f"\n--- Phase 1: baseline at {STEADY_STATE_OUT}% for {BASELINE_SECS}s ---")
    set_mode("manual", output=STEADY_STATE_OUT)
    baseline = poll_loop(BASELINE_SECS, "baseline")

    # Phase 2 — apply step
    print(f"\n--- Phase 2: step to {STEP_OUTPUT}% for {STEP_SECS}s ---")
    step_start_time = time.monotonic()
    set_mode("manual", output=STEP_OUTPUT)
    step = poll_loop(STEP_SECS, "step")

    # Return to PID control
    set_mode("pid")
    print("\nPID control restored.")

    # ── Save CSV ──────────────────────────────────────────────────────────────
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_path = f"step_test_{timestamp}.csv"

    with open(csv_path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["phase", "elapsed_s", "temperature_C", "output_pct"])
        for rec in baseline:
            w.writerow(["baseline", *rec])
        for rec in step:
            w.writerow(["step", *rec])

    print(f"Data saved to {csv_path}")

    # ── Ramp rate calculation ─────────────────────────────────────────────────
    step_times = np.array([r[0] for r in step])
    step_temps = np.array([r[1] for r in step])

    skip = int(len(step) * RAMP_SKIP_FRACTION)
    fit_t = step_times[skip:]
    fit_T = step_temps[skip:]

    coeffs = np.polyfit(fit_t, fit_T, 1)  # linear fit: T = a*t + b
    ramp_rate_per_sec = coeffs[0]          # °C/s (at full step output)

    delta_output = STEP_OUTPUT - STEADY_STATE_OUT
    # Integrating gain R: °C/s per 1% output
    R = ramp_rate_per_sec / delta_output

    print(f"\n=== Step response results ===")
    print(f"  Δoutput:    {delta_output:.1f}%")
    print(f"  Ramp rate:  {ramp_rate_per_sec*60:.4f} °C/min  ({ramp_rate_per_sec:.5f} °C/s)")
    print(f"  Process gain R = {R:.6f} °C/s per 1% output")

    # ── Lambda tuning ─────────────────────────────────────────────────────────
    print(f"\n=== Lambda PI tuning (PI, P_ON_M) ===")
    print(f"  {'λ (s)':>8}  {'Kp':>10}  {'Ki':>12}")
    print(f"  {'-'*8}  {'-'*10}  {'-'*12}")
    for lam in LAMBDAS:
        Kp = 1.0 / (R * lam)
        Ki = Kp / (2 * lam)
        print(f"  {lam:>8}  {Kp:>10.4f}  {Ki:>12.6f}")
    print()
    print("  Paste chosen values into config.h as DEFAULT_KP / DEFAULT_KI")
    print("  (DEFAULT_KD = 0.0, DEFAULT_P_ON_M = true)")

    # ── Plot ──────────────────────────────────────────────────────────────────
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 7), sharex=False)

    base_times = np.array([r[0] for r in baseline])
    base_temps = np.array([r[1] for r in baseline])

    # Offset step times so they continue from end of baseline on x-axis
    offset = base_times[-1] if len(base_times) else 0
    all_t = np.concatenate([base_times, step_times + offset])
    all_T = np.concatenate([base_temps, step_temps])
    step_boundary = offset

    ax1.plot(all_t, all_T, color="steelblue", linewidth=1.5, label="Temperature")
    ax1.axvline(step_boundary, color="orange", linestyle="--", label="Step applied")
    # Overlay the fitted ramp
    fit_t_abs = fit_t + offset
    ax1.plot(fit_t_abs, np.polyval(coeffs, fit_t), color="red",
             linestyle="--", linewidth=1, label=f"Ramp fit ({ramp_rate_per_sec*60:.3f} °C/min)")
    ax1.set_ylabel("Temperature (°C)")
    ax1.set_title("Boiler step test — temperature response")
    ax1.legend()
    ax1.grid(True, alpha=0.3)

    all_out = np.concatenate([[r[2] for r in baseline], [r[2] for r in step]])
    ax2.plot(all_t, all_out, color="tomato", linewidth=1.5)
    ax2.axvline(step_boundary, color="orange", linestyle="--")
    ax2.set_ylabel("Output (%)")
    ax2.set_xlabel("Elapsed time (s)")
    ax2.set_title("Heater output")
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plot_path = f"step_test_{timestamp}.png"
    plt.savefig(plot_path, dpi=150)
    print(f"Plot saved to {plot_path}")
    plt.show()


if __name__ == "__main__":
    main()
