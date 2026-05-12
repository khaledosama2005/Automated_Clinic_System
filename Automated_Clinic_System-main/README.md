# Smart Hospital Queue Management System

A hospital patient queue simulation built for the CSCE 2211 Applied Data Structures course at the American University in Cairo. The project demonstrates two fundamentally different data structures — a FIFO linked-list queue and a max-heap priority queue — working side by side in a realistic medical context, connected through a C++ backend engine, a Python bridge, and a browser-based frontend.

---

## What this project does

The core problem is simple: in a real hospital, you cannot serve patients in arrival order. A patient having a heart attack must be seen before someone with a minor cut, even if the minor cut patient got there first. But you also cannot ignore low-urgency patients forever — they need to be served eventually too.

This system handles both concerns. Patients with known conditions go directly into a priority queue where urgency determines order. Patients who do not know their condition (they select "Other") wait in a strict FIFO queue until a receptionist assesses them and assigns an urgency level. Once assessed, they join the priority queue and compete normally.

The result is a working simulation of a hospital triage system, with a live web frontend, an admin dashboard, and a C++ engine doing all the actual data structure operations.

---

## Architecture

The project is split into three layers that are completely independent of each other. Each layer does one job and hands off to the next.

**Layer 1 — C++ Engine**
All data structures, business logic, and medical decisions live here. The engine is compiled into a binary (`hospital_engine.exe` on Windows, `hospital_engine` on Linux/Mac). Every time it runs, it reads `state.json`, applies one action, writes the updated state back to `state.json`, prints a JSON response to stdout, and exits. It has no memory between calls — the file is its memory.

**Layer 2 — Python Bridge**
A single file (`bridge.py`) that is the only part of the codebase that knows the C++ binary exists. When the web server receives a request, the bridge spawns the binary as a subprocess, pipes a JSON command into its stdin, reads the JSON response from its stdout, and returns it as a Python dictionary.

**Layer 3 — Web Layer**
Django views and URL routing that expose the engine to a browser. Each view does exactly three things: parse the HTTP request body, call `run_command()` in the bridge, and return the result. There is also a zero-dependency demo server (`demo_server.py`) that does the same thing without needing Django installed.

```
Browser (index.html + app.js)
        |  HTTP JSON
Django views.py / demo_server.py
        |  Python function call
bridge.py  <-- only file that knows C++ exists
        |  subprocess stdin / stdout
hospital_engine.exe
        |  reads and writes
state.json
```

---

## Data Structures

### FIFO Queue — Queue.h / Queue.cpp

A singly-linked list implementation. Each node holds a `Patient*` and a pointer to the next node. The queue tracks `front` and `rear` pointers.

- Enqueue: O(1) — appends a new node at the rear
- Dequeue: O(1) — removes the front node and advances the pointer
- Used exclusively for patients who select "Other" (urgency unknown)

The linked list was chosen over a circular array because it has no fixed capacity — it grows dynamically with each patient.

### Max-Heap Priority Queue — PriorityQueue.h

A max-heap backed by a `std::vector<Patient*>`. The heap invariant means the patient with the highest priority score is always at the root (`heap[0]`).

- Insert: O(log n) — appends to the end, then sifts up by swapping with parent until in correct position
- Extract max: O(log n) — removes the root, moves the last element to the top, then sifts down
- Aging rebuild: O(n) — called each simulation tick, updates all scores and rebuilds the heap using bottom-up heapify
- One heap per clinic — patients compete within their department, not globally

---

## Priority Formula

Every patient's priority score is computed the moment they have a known condition:

```
priorityScore = urgencyLevel x 1,000,000
              - arrivalTime
              + agingCounter x 100
```

This formula encodes three rules that never conflict with each other.

**Rule 1 — Urgency always dominates.** The gap of one million between urgency levels means no amount of waiting can ever let a lower-urgency patient overtake a higher-urgency one. A Critical patient (base 5,000,000) will always outrank an Urgent patient (base 4,000,000) regardless of how long either has waited.

**Rule 2 — FIFO tiebreaking within the same urgency.** Within the same urgency level, the patient who arrived earlier has a smaller `arrivalTime`, giving them a higher score. Earlier arrival wins ties.

**Rule 3 — Aging prevents starvation.** The `agingCounter` increments every simulation tick and adds 100 to the score each time. This slowly boosts the priority of patients who have been waiting a long time within their urgency tier. The 100-per-tick increment is intentionally small — it closes gaps between patients of the same urgency quickly, but crossing an urgency boundary would require waiting roughly 10,000 ticks, which maintains clinical correctness.

Patients with `UrgencyLevel::PENDING` (selected "Other", not yet assessed) have a priority score of -1 and are not eligible for the heap.

---

## Patient Flow

### Normal patient (preset condition)

1. Patient logs in, selects a clinic, selects a preset condition
2. Engine creates a `Patient` object, calls `computePriority()`, inserts into the clinic's max-heap
3. Patient is immediately competing for service in priority order

### "Other" patient (unknown condition)

1. Patient logs in, selects a clinic, selects "Other", types their symptoms
2. Engine creates a `Patient` with `UrgencyLevel::PENDING`, priority score = -1
3. Patient ID is appended to the `pendingIds` list in strict FIFO order
4. Admin opens the triage panel, sees the patient's reported symptoms
5. Admin assigns an urgency level and estimated treatment time, clicks Triage
6. Engine calls `computePriority()` — patient now has a real score
7. Patient is inserted into the clinic's max-heap and competes normally

### Serving a patient

1. Admin clicks "Finish Next" for a clinic
2. Engine calls `extractMax()` on that clinic's heap
3. Root (highest priority patient) is removed in O(log n)
4. Patient is marked as served with a timestamp

---

## File Structure

```
project-root/
|
|-- main.cpp                  Entry point — reads stdin, calls engine, prints stdout
|-- HospitalEngine.h          Engine class declaration
|-- HospitalEngine.cpp        Engine implementation — all API actions, serialization
|-- Patient.h                 Patient class — priority formula, state
|-- User.h                    Abstract base class for Patient, Doctor, Receptionist
|-- Category.h                UrgencyLevel enum and Category struct
|-- Condition.h               Medical condition struct (name + urgency + treatment time)
|-- Clinic.h                  Clinic class — owns a PriorityQueue and preset conditions
|-- PriorityQueue.h           Max-heap of Patient* — the main priority data structure
|-- Queue.h                   FIFO linked-list queue declaration
|-- Queue.cpp                 FIFO linked-list queue implementation
|-- pq.cpp                    Standalone simplified priority queue (pedagogical reference)
|-- Doctor.h                  Doctor class — availability countdown timer
|-- Receptionist.h            Receptionist class — composes the FIFO queue
|-- HASHMAP.h                 Custom hash map with DJB2 hashing and separate chaining
|-- SimpleJson.h              Custom JSON parser and serializer (no external dependencies)
|-- state.json                Persistent state — read and written on every binary call
|
|-- hospital_backend/
|   |-- bridge.py             The only Python file that knows C++ exists
|   |-- views.py              Django views — one function per API endpoint
|   |-- urls.py               URL routing
|   `-- settings.py           Django configuration
|
|-- demo_server.py            Zero-dependency HTTP server (alternative to Django)
|
`-- demo/
    |-- index.html            Frontend — login screen and app screen
    |-- app.js                Frontend logic — API calls, rendering, event handlers
    `-- style.css             Styling — dark theme, responsive grid layout
```

---

## File-by-File Reference

**Category.h**
Defines the `UrgencyLevel` enum with six values: `PENDING=0`, `ELECTIVE=1`, `NON_URGENT=2`, `SEMI_URGENT=3`, `URGENT=4`, `CRITICAL=5`. Also defines the `Category` struct which wraps an urgency level with a human-readable name and a maximum wait time threshold. These integer values feed directly into the priority formula.

**Condition.h**
A struct that pairs a `Category` with a condition name and an estimated treatment duration in minutes. Each clinic stores a list of preset conditions that patients can select. If a patient picks "Other", they get a default `Condition` with `UrgencyLevel::PENDING`.

**User.h**
Abstract base class with three fields shared by all actors: `id`, `name`, and `UserRole` (PATIENT, DOCTOR, RECEPTIONIST). Declares one pure virtual method `displayInfo()`, making it impossible to instantiate directly. Patient, Doctor, and Receptionist all inherit from it.

**Patient.h**
The central entity. Inherits from `User`. Key fields are `arrivalTime` (the global sequence counter at registration time), `agingCounter` (incremented each tick), `priorityScore` (computed by the formula), `condition`, and `served`. The `computePriority()` method implements the three-rule formula and is called automatically whenever `setCondition()` is called.

**Queue.h / Queue.cpp**
Singly-linked list FIFO queue. Holds `Patient*` pointers — does not own the Patient objects. The destructor frees all nodes but not the patients themselves, since ownership belongs to the engine. Used by `Receptionist` for the pending triage queue.

**PriorityQueue.h**
Max-heap backed by `std::vector<Patient*>`. The `higher()` comparator compares `priorityScore` values. `siftUp()` restores the heap after insertion. `siftDown()` restores it after extraction. `applyAgingAndRebuild()` increments aging on every patient then calls `heapify()` (bottom-up O(n) rebuild) to reflect the updated scores.

**HASHMAP.h**
Custom hash map using separate chaining. The hash function is DJB2: starts at 5381, then for each character `h = h * 33 + c`. Rehashes automatically when the load factor exceeds 0.75 by doubling capacity and reinserting all entries.

**SimpleJson.h**
Header-only recursive descent JSON parser and serializer. Supports objects, arrays, strings, numbers, booleans, and null. Used for all communication between the binary and the Python bridge, and for reading and writing `state.json`. No external libraries required.

**Clinic.h**
Represents one hospital department. Owns a `PriorityQueue`, a list of `Doctor*`, and a list of preset `Condition` objects. Provides `enqueuePatient()`, `dequeuePatient()`, and `applyAgingTick()`. Tracks statistics including total patients served, average wait time, and throughput.

**Doctor.h**
Inherits from `User`. The key field is `minutesUntilFree`, a countdown timer. `assignPatient(minutes)` marks the doctor as busy and starts the countdown. `tick()` decrements the counter each simulation step and releases the doctor when it hits zero.

**Receptionist.h**
Inherits from `User` and composes a `Queue` internally. `receivePendingPatient()` enqueues a patient in FIFO order. `assignUrgencyAndRoute()` dequeues the front patient, sets their condition, and routes them to the correct clinic's heap. This is the exact transition point from the FIFO world to the priority queue world.

**HospitalEngine.h / HospitalEngine.cpp**
The brain of the system. Owns all data: `vector<unique_ptr<Clinic>>`, `unordered_map<int, unique_ptr<Patient>>`, and `vector<int> pendingIds`. The `handle()` method dispatches action strings to private `api_*` methods. `load()` reconstructs full state from `state.json` on startup. `save()` serializes everything back after every mutation. On corrupt or missing state, it gracefully falls back to a clean default setup with three clinics.

**main.cpp**
The entry point. Reads one JSON line from stdin, parses it, creates a `HospitalEngine`, calls `load()`, dispatches with `handle()`, prints the response to stdout, and exits. Handles all error cases — empty input, invalid JSON, missing action field — by always printing a valid JSON error object so the Python bridge always has something to parse.

**bridge.py**
Locates the binary by walking the repo directory tree and picking the most recently compiled one (by modification time). `run_command()` serializes the action and data to JSON, spawns the binary as a subprocess, pipes the command to stdin, captures stdout, parses the JSON response, and returns a Python dict. Never raises an exception — all failure paths return `{"status": "error", "message": "..."}`. `diagnose()` provides a health snapshot without touching C++.

**views.py**
One function per API endpoint. Every function does exactly three things: parse the request body, call `run_command()`, return `JsonResponse`. There is zero business logic here.

**urls.py**
Routes all `/api/...` requests to the hospital app's view functions. The top-level router delegates to the app's own URL file using `include()`.

**settings.py**
Standard Django configuration. Uses SQLite for Django's internal auth and session tables. The actual hospital data lives in `state.json`, not the database.

**demo_server.py**
A zero-dependency HTTP server using Python's built-in `ThreadingHTTPServer`. Exposes the same API routes as Django and also serves the frontend HTML, CSS, and JS from the `demo/` folder. Use this if you do not want to install Django.

**index.html**
Two screens: a login screen and an app screen. The app screen has a patient card (clinic selector, symptom selector, queue position display) and an admin card (clinic management, symptom management, finish next, triage pending). All interactive elements have `id` attributes that `app.js` references directly.

**app.js**
The `api()` helper wraps `fetch()` for all HTTP calls. `loadClinics()` populates all dropdowns from the C++ clinic data. `refreshState()` fetches the current queue state and calls `renderState()` to redraw the patient list. `init()` wires up all button event listeners. Login is client-side only — entering the name "Admin" with an empty phone field grants admin access.

**style.css**
Dark theme built with CSS custom properties. Urgency badges are color-coded: red for Critical, orange for Urgent, blue for Normal. Two-column grid layout that collapses to a single column on narrow screens.

---

## API Reference

All endpoints accept and return JSON. Every response has either `"status": "ok"` with a `"data"` field, or `"status": "error"` with a `"message"` field.

| Method | Path | Description | Request Body |
|--------|------|-------------|--------------|
| GET | `/api/clinics` | List all clinics with their preset conditions | — |
| GET | `/api/state` | Full queue state — all clinics, waiting patients, pending list | — |
| GET | `/api/status` | Health check — binary path, exists, executable, state file exists | — |
| POST | `/api/add_patient` | Register a patient with a preset condition | `{name, phone?, clinicId, symptomIndex}` |
| POST | `/api/add_pending_patient` | Register a patient with unknown condition | `{name, phone?, clinicId, symptomsText}` |
| POST | `/api/admin/add_clinic` | Add a new clinic | `{name, specialty?}` |
| POST | `/api/admin/remove_clinic` | Remove a clinic by ID | `{clinicId}` |
| POST | `/api/admin/add_symptom` | Add a preset condition to a clinic | `{clinicId, name, urgencyLevel, estimatedTreatmentMinutes}` |
| POST | `/api/admin/remove_symptom` | Remove a preset condition by index | `{clinicId, symptomIndex}` |
| POST | `/api/admin/finish_next` | Serve the highest-priority patient in a clinic | `{clinicId}` |
| POST | `/api/admin/triage_pending` | Assign urgency to the next FIFO patient and route them to the heap | `{clinicId, urgencyLevel, estimatedTreatmentMinutes}` |
| POST | `/api/reset` | Wipe all state and return to default setup | `{}` |

---

## Setup and Running

### Prerequisites

- A C++17 compiler (g++ recommended)
- Python 3.9 or higher
- Django 4+ (only if using the Django server, not required for demo server)

### Step 1 — Compile the C++ engine

```bash
g++ -std=c++17 -O2 main.cpp Queue.cpp HospitalEngine.cpp -o hospital_engine
```

On Windows:
```bash
g++ -std=c++17 -O2 main.cpp Queue.cpp HospitalEngine.cpp -o hospital_engine.exe
```

On Linux/Mac, mark it as executable after compiling:
```bash
chmod +x hospital_engine
```

### Step 2a — Run with the demo server (recommended, no Django needed)

```bash
python demo_server.py
```

Open your browser at `http://127.0.0.1:8080/`

### Step 2b — Run with Django

```bash
cd hospital_backend
pip install django
python manage.py migrate
python manage.py runserver
```

Open your browser at `http://127.0.0.1:8000/`

### Verifying the setup

Hit the status endpoint to confirm everything is wired up correctly:

```bash
curl http://127.0.0.1:8080/api/status
```

The response shows whether the binary was found, whether it is executable, and whether `state.json` exists. If anything is wrong, the message tells you exactly what to fix.

### Testing the binary directly from the terminal

You can talk to the C++ engine without any Python at all:

```bash
echo '{"action":"get_clinics","data":{}}' | ./hospital_engine --state state.json
```

This is the exact same thing the bridge does programmatically on every request.

---

## Using the Frontend

**As a patient:**
1. Enter your name and phone number, click Sign In
2. Select your clinic from the dropdown
3. Select your condition from the list, or select "Other" and describe your symptoms in the text box
4. Click Join Queue
5. Your position in the queue appears below and updates whenever you refresh

**As admin:**
1. Enter the name `Admin` with an empty phone field, click Sign In
2. The admin dashboard appears with full system controls
3. Use **Finish Next** to dequeue and serve the highest-priority patient from any clinic
4. Use the **Pending Triage** panel to handle "Other" patients — read their reported symptoms, assign a severity level and estimated treatment time, click Triage to move them into the priority queue
5. Use the Clinics and Symptoms panels to add or remove departments and preset conditions at any time
6. Click **Reset** to wipe all state and start fresh — useful before a demo or grading session

---

## Design Notes

**Why does the binary exit after every request?**
Keeping a long-running binary would require thread safety across concurrent requests. By serializing all state to a file and reloading it each time, every binary invocation is completely independent. The tradeoff is subprocess overhead per request, which is acceptable for a simulation system.

**Why `unique_ptr` in the engine but raw pointers in the queues?**
The engine's `unordered_map` is the single owner of every Patient object. The clinic heaps and the pending list hold only raw pointers — they observe patients but do not own them. This means a patient can be referenced from multiple places simultaneously with no duplication and no memory leak. One owner, many observers.

**Why is the aging increment 100 and not something larger?**
The urgency gap between levels is one million. An aging increment of 100 means a patient needs to wait 10,000 ticks to close a full urgency gap. This is intentional — aging should prevent starvation within a tier quickly, but should never allow a genuinely lower-urgency patient to overtake a higher-urgency one in any realistic scenario.

**Why does the admin login check happen in JavaScript and not on the server?**
This is a demo convenience only. The system has no real authentication layer. For a production deployment, this would need to be replaced with proper server-side authentication.
