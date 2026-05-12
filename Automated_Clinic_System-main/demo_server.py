"""
demo_server.py
==============
Zero-dependency demo frontend server (no Django required).

Serves:
  GET  /            -> demo/index.html
  GET  /assets/*    -> demo static assets

API:
  GET  /api/status
  GET  /api/clinics
  GET  /api/state
  POST /api/add_patient
  POST /api/add_pending_patient
  POST /api/reset

Admin API:
  POST /api/admin/add_clinic
  POST /api/admin/remove_clinic
  POST /api/admin/add_symptom
  POST /api/admin/remove_symptom
  POST /api/admin/finish_next
  POST /api/admin/triage_pending

All API calls delegate to hospital_backend/bridge.py (subprocess to C++).
"""

from __future__ import annotations

import json
import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

from hospital_backend.bridge import run_command, diagnose


ROOT = os.path.dirname(os.path.abspath(__file__))
DEMO_DIR = os.path.join(ROOT, "demo")


def _read_json(handler: BaseHTTPRequestHandler) -> dict:
    length = int(handler.headers.get("Content-Length") or "0")
    if length <= 0:
        return {}
    raw = handler.rfile.read(length).decode("utf-8", errors="replace")
    try:
        return json.loads(raw) if raw.strip() else {}
    except json.JSONDecodeError:
        return {}


def _send_json(handler: BaseHTTPRequestHandler, payload: dict, status: int = 200) -> None:
    body = json.dumps(payload).encode("utf-8")
    handler.send_response(status)
    handler.send_header("Content-Type", "application/json; charset=utf-8")
    handler.send_header("Content-Length", str(len(body)))
    handler.send_header("Access-Control-Allow-Origin", "*")
    handler.send_header("Access-Control-Allow-Headers", "Content-Type")
    handler.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
    handler.end_headers()
    handler.wfile.write(body)


def _send_file(handler: BaseHTTPRequestHandler, path: str) -> None:
    if not os.path.isfile(path):
        handler.send_response(404)
        handler.end_headers()
        return

    ext = os.path.splitext(path)[1].lower()
    ctype = "text/plain; charset=utf-8"
    if ext == ".html":
        ctype = "text/html; charset=utf-8"
    elif ext == ".css":
        ctype = "text/css; charset=utf-8"
    elif ext == ".js":
        ctype = "application/javascript; charset=utf-8"

    with open(path, "rb") as f:
        data = f.read()

    handler.send_response(200)
    handler.send_header("Content-Type", ctype)
    handler.send_header("Content-Length", str(len(data)))
    handler.end_headers()
    handler.wfile.write(data)


class Handler(BaseHTTPRequestHandler):
    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.end_headers()

    def do_GET(self):
        path = urlparse(self.path).path

        if path == "/" or path == "/demo":
            return _send_file(self, os.path.join(DEMO_DIR, "index.html"))
        if path.startswith("/assets/"):
            rel = os.path.normpath(path[len("/assets/"):].lstrip("/"))
            full = os.path.join(DEMO_DIR, rel)
            # Keep it inside demo/
            if not os.path.abspath(full).startswith(os.path.abspath(DEMO_DIR)):
                self.send_response(403)
                self.end_headers()
                return
            return _send_file(self, full)

        # API
        if path == "/api/status":
            return _send_json(self, {"status": "ok", "data": diagnose()})
        if path == "/api/clinics":
            return _send_json(self, run_command("get_clinics"))
        if path == "/api/state":
            return _send_json(self, run_command("get_state"))

        self.send_response(404)
        self.end_headers()

    def do_POST(self):
        path = urlparse(self.path).path
        data = _read_json(self)

        if path == "/api/add_patient":
            return _send_json(self, run_command("add_patient", data))
        if path == "/api/add_pending_patient":
            return _send_json(self, run_command("add_pending_patient", data))
        if path == "/api/reset":
            return _send_json(self, run_command("reset", {}))
        
        # Admin
        if path == "/api/admin/add_clinic":
            return _send_json(self, run_command("admin_add_clinic", data))
        if path == "/api/admin/remove_clinic":
            return _send_json(self, run_command("admin_remove_clinic", data))
        if path == "/api/admin/add_symptom":
            return _send_json(self, run_command("admin_add_symptom", data))
        if path == "/api/admin/remove_symptom":
            return _send_json(self, run_command("admin_remove_symptom", data))
        if path == "/api/admin/finish_next":
            return _send_json(self, run_command("admin_finish_next", data))
        if path == "/api/admin/triage_pending":
            return _send_json(self, run_command("admin_triage_pending", data))

        self.send_response(404)
        self.end_headers()

    def log_message(self, format: str, *args) -> None:
        # quieter console
        return


def main() -> None:
    host = "127.0.0.1"
    port = 8080
    httpd = ThreadingHTTPServer((host, port), Handler)
    print(f"Demo UI: http://{host}:{port}/")
    print("If API errors mention missing binary, compile hospital_engine.exe first.")
    httpd.serve_forever()


if __name__ == "__main__":
    main()

