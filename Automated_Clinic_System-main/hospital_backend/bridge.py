"""
bridge.py
=========
The ONLY file in this entire project that knows C++ exists.
Every Django view calls run_command() and receives a plain Python dict.
No view ever touches subprocess, file paths, or JSON encoding directly.
"""

import subprocess
import json
import os
import sys

# ---------------------------------------------------------------------------
# Path resolution
# ---------------------------------------------------------------------------
# bridge.py is at:  ADS/hospital_backend/hospital/bridge.py
# We want:          ADS/Automated_Clinic_System/
#
#   dirname(__file__)        → hospital/
#   dirname(dirname)         → hospital_backend/
#   join(.., "..")           → ADS/
#   join(.., "Automated_..") → ADS/Automated_Clinic_System/

_HERE         = os.path.abspath(__file__)
# bridge.py lives in: <repo>/hospital_backend/bridge.py
# The C++ binary lives in the repo root (same level as hospital_backend/).
_REPO_ROOT    = os.path.dirname(os.path.dirname(_HERE))
_CPP_DIR      = os.path.normpath(_REPO_ROOT)

# Binary name is OS-dependent
_BIN_NAME = "hospital_engine.exe" if sys.platform == "win32" else "hospital_engine"

# Public so views can import them for the /api/status/ health-check
def _find_binary() -> str:
    """
    Locate the newest hospital_engine binary under the repo root.
    This avoids confusing behavior when multiple copies exist.
    """
    candidates = []
    for root, _dirs, files in os.walk(_CPP_DIR):
        if _BIN_NAME in files:
            p = os.path.join(root, _BIN_NAME)
            try:
                candidates.append((os.path.getmtime(p), p))
            except OSError:
                continue
    if not candidates:
        # default expected location
        return os.path.join(_CPP_DIR, _BIN_NAME)
    candidates.sort(reverse=True)
    return candidates[0][1]


BINARY = _find_binary()
STATE  = os.path.join(os.path.dirname(BINARY), "state.json")


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def run_command(action: str, data: dict = None) -> dict:
    """
    Send one JSON command to the C++ binary via stdin.
    Return the binary's JSON response as a Python dict.

    stdin  → {"action": "add_patient", "data": {"name": "Sara", ...}}
    stdout ← {"status": "ok",    "data": { ... }}
           or {"status": "error", "message": "..."}

    This function NEVER raises. On any failure it returns:
        {"status": "error", "message": "<reason>"}
    Views can always safely check  result["status"] == "ok".
    """
    if data is None:
        data = {}

    # Serialize the command
    try:
        cmd_str = json.dumps({"action": action, "data": data})
    except (TypeError, ValueError) as exc:
        return {"status": "error", "message": f"Could not serialize command: {exc}"}

    binary = _find_binary()
    state_path = os.path.join(os.path.dirname(binary), "state.json")

    # Guard: binary must exist before we try to run it
    if not os.path.isfile(binary):
        return {
            "status": "error",
            "message": (
                f"Binary not found at '{binary}'. "
                "Compile first in the C++ folder: "
                "g++ -std=c++17 -O2 main.cpp Queue.cpp HospitalEngine.cpp -o hospital_engine"
            )
        }

    # Run the binary
    try:
        proc = subprocess.run(
            [binary, "--state", state_path],
            input=cmd_str,
            capture_output=True,
            text=True,
            timeout=10        # hard cap — a stuck binary won't hang Django
        )
    except subprocess.TimeoutExpired:
        return {"status": "error", "message": "C++ binary timed out (10 s)."}
    except PermissionError:
        return {
            "status": "error",
            "message": (
                f"Cannot execute '{BINARY}'. "
                "On Linux/Mac run: chmod +x Automated_Clinic_System/hospital_engine"
            )
        }
    except Exception as exc:
        return {"status": "error", "message": f"Subprocess error: {exc}"}

    # Non-zero exit = binary crashed or hit a fatal error
    if proc.returncode != 0:
        msg = proc.stderr.strip() or f"Binary exited with code {proc.returncode}."
        return {"status": "error", "message": msg}

    stdout = proc.stdout.strip()

    if not stdout:
        return {
            "status": "error",
            "message": (
                "Binary produced no output. "
                "Make sure main() prints exactly one JSON object to stdout "
                "and nothing else (no stray cout debug prints)."
            )
        }

    # Parse JSON — surface raw output so the C++ author can debug
    try:
        return json.loads(stdout)
    except json.JSONDecodeError as exc:
        return {
            "status": "error",
            "message": (
                f"Binary returned invalid JSON ({exc}). "
                f"Raw output (first 400 chars): {stdout[:400]}"
            )
        }


# ---------------------------------------------------------------------------
# Diagnostic helper
# ---------------------------------------------------------------------------

def diagnose() -> dict:
    """
    Returns a snapshot of the bridge configuration.
    Call from /api/status/ to verify setup without touching any C++.

    Quick check from the terminal:
        cd hospital_backend
        python manage.py shell
        >>> from hospital.bridge import diagnose
        >>> import pprint; pprint.pprint(diagnose())
    """
    binary = _find_binary()
    state_path = os.path.join(os.path.dirname(binary), "state.json")
    try:
        mtime = os.path.getmtime(binary) if os.path.isfile(binary) else None
    except OSError:
        mtime = None

    return {
        "platform":     sys.platform,
        "cppDir":       _CPP_DIR,
        "cppDirExists": os.path.isdir(_CPP_DIR),
        "binaryPath":   binary,
        "binaryExists": os.path.isfile(binary),
        "binaryMtime":  mtime,
        "binaryExec":   os.access(binary, os.X_OK) if os.path.isfile(binary) else False,
        "statePath":    state_path,
        "stateExists":  os.path.isfile(state_path),
    }
