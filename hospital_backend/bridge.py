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
_DJANGO_ROOT  = os.path.dirname(os.path.dirname(_HERE))   # hospital_backend/
_ADS_ROOT     = os.path.dirname(_DJANGO_ROOT)              # ADS/
_CPP_DIR      = os.path.join(_ADS_ROOT, "Automated_Clinic_System")
_CPP_DIR      = os.path.normpath(_CPP_DIR)

# Binary name is OS-dependent
_BIN_NAME = "hospital_engine.exe" if sys.platform == "win32" else "hospital_engine"

# Public so views can import them for the /api/status/ health-check
BINARY = os.path.join(_CPP_DIR, _BIN_NAME)
STATE  = os.path.join(_CPP_DIR, "state.json")


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

    # Guard: binary must exist before we try to run it
    if not os.path.isfile(BINARY):
        return {
            "status": "error",
            "message": (
                f"Binary not found at '{BINARY}'. "
                "Compile first inside Automated_Clinic_System/: "
                "g++ *.cpp -o hospital_engine"
            )
        }

    # Run the binary
    try:
        proc = subprocess.run(
            [BINARY, "--state", STATE],
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
    return {
        "platform":     sys.platform,
        "cppDir":       _CPP_DIR,
        "cppDirExists": os.path.isdir(_CPP_DIR),
        "binaryPath":   BINARY,
        "binaryExists": os.path.isfile(BINARY),
        "binaryExec":   os.access(BINARY, os.X_OK) if os.path.isfile(BINARY) else False,
        "statePath":    STATE,
        "stateExists":  os.path.isfile(STATE),
    }
