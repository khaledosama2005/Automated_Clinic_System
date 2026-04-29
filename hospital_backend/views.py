"""
views.py
========
One function per API endpoint. Every function is the same 3 steps:
  1. Parse the JSON body from the browser request
  2. Call run_command(action, data)
  3. Return the result as a JsonResponse

Zero business logic lives here — that is all in the C++ binary.
"""

import json
from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt
from django.views.decorators.http import require_http_methods
from .bridge import run_command, diagnose


def _body(request) -> dict:
    """Parse JSON request body. Returns {} on empty or malformed body."""
    if not request.body:
        return {}
    try:
        return json.loads(request.body)
    except json.JSONDecodeError:
        return {}


# ── Health check ──────────────────────────────────────────────────────────────
# GET /api/status/
# Hit this first after setup — tells you if paths are correct before C++ is ready.
@require_http_methods(["GET"])
def status(request):
    return JsonResponse({"status": "ok", "data": diagnose()})


# ── Read-only endpoints (GET) ─────────────────────────────────────────────────

# GET /api/clinics/
# Returns clinic list + preset conditions for each.
# Frontend needs this to build the "select your condition" dropdown.
@require_http_methods(["GET"])
def clinics(request):
    return JsonResponse(run_command("get_clinics"))


# GET /api/queue_state/
# Returns live state of every clinic queue + receptionist pending queue.
# Dashboard polls this every few seconds.
@require_http_methods(["GET"])
def queue_state(request):
    return JsonResponse(run_command("get_queue_state"))


# GET /api/stats/
# Returns avg wait, throughput, FIFO vs priority comparison numbers.
@require_http_methods(["GET"])
def stats(request):
    return JsonResponse(run_command("get_stats"))


# ── Patient registration (POST) ───────────────────────────────────────────────

# POST /api/add_patient/
# Patient picked a preset condition.
# Body: { "name": "Sara Ahmed", "clinicId": 2, "conditionIndex": 1 }
@csrf_exempt
@require_http_methods(["POST"])
def add_patient(request):
    return JsonResponse(run_command("add_patient", _body(request)))


# POST /api/add_pending_patient/
# Patient picked "Other" → goes to receptionist FIFO queue.
# Body: { "name": "Nour Adel", "clinicId": 1 }
@csrf_exempt
@require_http_methods(["POST"])
def add_pending_patient(request):
    return JsonResponse(run_command("add_pending_patient", _body(request)))


# ── Receptionist actions (POST) ───────────────────────────────────────────────

# POST /api/triage_pending/
# Receptionist assigns urgency to the next FIFO patient and routes them.
# Body: { "urgencyLevel": 4, "estimatedTreatmentMinutes": 30, "clinicId": 1 }
@csrf_exempt
@require_http_methods(["POST"])
def triage_pending(request):
    return JsonResponse(run_command("triage_pending", _body(request)))


# ── Simulation controls (POST) ────────────────────────────────────────────────

# POST /api/serve_next/
# Dequeue highest-priority patient from a clinic and assign to a free doctor.
# Body: { "clinicId": 1 }
@csrf_exempt
@require_http_methods(["POST"])
def serve_next(request):
    return JsonResponse(run_command("serve_next", _body(request)))


# POST /api/tick/
# Advance simulation by one tick: doctor countdowns, aging, assignments.
# Body: {}
@csrf_exempt
@require_http_methods(["POST"])
def tick(request):
    return JsonResponse(run_command("tick", {}))


# POST /api/reset/
# Wipe state.json and return to a clean system. Use before demos/grading.
# Body: {}
@csrf_exempt
@require_http_methods(["POST"])
def reset(request):
    return JsonResponse(run_command("reset", {}))
