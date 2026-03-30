# Priority-Based Service Queue Manager
### CSCE 2211 – Applied Data Structures | Project 8

---

## Table of Contents
1. [Project Overview](#project-overview)
2. [System Architecture](#system-architecture)
3. [File Structure](#file-structure)
4. [Class Reference](#class-reference)
   - [Category](#category)
   - [Condition](#condition)
   - [User](#user)
   - [Patient](#patient)
   - [Doctor](#doctor)
   - [PriorityQueue](#priorityqueue)
   - [Clinic](#clinic)
   - [Receptionist](#receptionist)
   - [PriorityQueueManager](#priorityqueuemanager)
5. [Data Flow](#data-flow)
6. [Priority Formula](#priority-formula)
7. [Complexity Analysis](#complexity-analysis)
8. [Challenge Section](#challenge-section)
9. [How to Compile and Run](#how-to-compile-and-run)

---

## Project Overview

This project simulates a real hospital service system where patients are processed based on medical urgency, not just arrival order. The system demonstrates:

- A **max-heap priority queue** that always serves the most critical patient first
- A **FIFO queue** at the receptionist desk for patients who need manual triage
- **Dynamic priority updates** with O(log n) complexity using a HashMap position map
- **Starvation prevention** through an aging mechanism that gradually boosts priority for long-waiting patients
- **Multi-level priority** — patients compete within their own clinic, not across all departments

---

## System Architecture

```
User (abstract base)
 ├── Patient       →  lives inside Clinic's PriorityQueue
 ├── Doctor        →  assigned to a Clinic, serves patients one at a time
 └── Receptionist  →  owns a FIFO pending queue for "Other" patients

Category            →  urgency enum with metadata (maxWait, priorityValue)
Condition           →  medical condition: name + Category + estimatedTreatmentMinutes

Clinic              →  one hospital department
 ├── PriorityQueue<Patient*>   (max-heap, per-clinic)
 ├── vector<Doctor*>
 └── vector<Condition>         (preset menu shown to patients)

PriorityQueue       →  max-heap backed by HashMap positionMap for O(log n) updates

PriorityQueueManager  →  orchestrator: routes patients, runs ticks, computes stats
```

---

## File Structure

```
Category.h              — UrgencyLevel enum + Category struct
Condition.h             — Condition struct (name, category, treatment time)
User.h                  — Abstract base class for all system actors
Patient.h               — Patient class (inherits User)
Patient.cpp             — Patient::displayInfo() implementation
Doctor.h                — Doctor class (inherits User)
PriorityQueue.h         — Max-heap with HashMap positionMap
Clinic.h                — Hospital department class
Receptionist.h          — Receptionist class with FIFO pending queue
PriorityQueueManager.h  — Central orchestrator
HASHMAP.h               — HashMap (provided by teammates)
Queue.h / Queue.cpp     — FIFO Queue (provided by teammates)
HospitalDemo.cpp        — Full system demo
main_test.cpp           — 171-test unit test suite
```

---

## Class Reference

---

### Category

**File:** `Category.h`

Wraps the `UrgencyLevel` enum with human-readable metadata. Every `Condition` holds a `Category`, and every `Patient`'s priority score is computed from their condition's category.

#### UrgencyLevel Enum

| Value       | Integer | Name         | Max Wait (min) | Description                        |
|-------------|---------|--------------|----------------|------------------------------------|
| PENDING     | 0       | Pending      | -1             | Awaiting receptionist assessment   |
| ELECTIVE    | 1       | Elective     | 240            | Routine, no urgency                |
| NON_URGENT  | 2       | Non-Urgent   | 120            | Can wait, not dangerous            |
| SEMI_URGENT | 3       | Semi-Urgent  | 60             | Should be seen within the hour     |
| URGENT      | 4       | Urgent       | 30             | Needs prompt attention             |
| CRITICAL    | 5       | Critical     | 0              | Immediate life-threatening care    |

#### Key Methods

| Method | Returns | Description |
|--------|---------|-------------|
| `Category(UrgencyLevel)` | — | Constructor; sets name and maxWait automatically |
| `getPriorityValue()` | `int` | Returns the integer value of the urgency level (0–5) |
| `isPending()` | `bool` | Returns true if the level is PENDING |

---

### Condition

**File:** `Condition.h`

Represents a medical condition a patient can select from the clinic menu, or the default "Other" condition awaiting receptionist triage.

#### Fields

| Field | Type | Description |
|-------|------|-------------|
| `name` | `string` | Display name, e.g. "Chest Pain" |
| `category` | `Category` | Contains urgency level + metadata |
| `estimatedTreatmentMinutes` | `int` | Used to set Doctor::minutesUntilFree on assignment |

#### Constructors

```cpp
Condition();                                          // "Other" — PENDING, 0 min
Condition(string name, UrgencyLevel lvl, int time);   // Preset condition
```

---

### User

**File:** `User.h`

Abstract base class for all system actors. Uses polymorphism so the manager can store and display any actor type uniformly.

#### Fields (protected)

| Field | Type | Description |
|-------|------|-------------|
| `id` | `int` | Unique identifier |
| `name` | `string` | Display name |
| `role` | `UserRole` | PATIENT / DOCTOR / RECEPTIONIST |

#### UserRole Enum

```cpp
enum class UserRole { PATIENT, DOCTOR, RECEPTIONIST };
```

#### Pure Virtual Method

```cpp
virtual void displayInfo() const = 0;
```

Every subclass must implement `displayInfo()` to print its details.

---

### Patient

**File:** `Patient.h`, `Patient.cpp`

The core entity managed by the entire system. Inherits from `User`. A Patient lives inside a `Clinic`'s `PriorityQueue` (once confirmed), or inside the `Receptionist`'s FIFO pending queue (if they chose "Other").

#### Key Fields

| Field | Type | Description |
|-------|------|-------------|
| `condition` | `Condition` | Set on registration or after receptionist triage |
| `arrivalTime` | `int` | Simulation tick when the patient registered |
| `agingCounter` | `int` | Increments each tick; boosts priority to prevent starvation |
| `priorityScore` | `int` | Computed value used by the heap for ordering |
| `serviceStartTime` | `int` | Tick when a doctor began seeing them |
| `waitTime` | `int` | `serviceStartTime - arrivalTime` |

#### Key Methods

| Method | Description |
|--------|-------------|
| `setCondition(Condition)` | Sets condition and immediately recomputes priority score |
| `incrementAging()` | Increments `agingCounter` by 1, recomputes priority score |
| `markServed(int tick)` | Records service start time and computes final wait time |
| `computePriority()` | Internal — recalculates `priorityScore` from the formula |
| `isPending()` | Returns true if condition is still PENDING |
| `getPriorityScore()` | Returns current computed priority score |

#### Priority Formula

```
priorityScore = (urgencyLevel × 1,000,000) - arrivalTime + (agingCounter × 100)
```

See the [Priority Formula](#priority-formula) section for a full explanation.

---

### Doctor

**File:** `Doctor.h`

Inherits from `User`. Each Doctor belongs to one Clinic. When a Patient is assigned, `minutesUntilFree` is set from the patient's `condition.estimatedTreatmentMinutes`. Each simulation `tick()` decrements this countdown until the doctor is available again.

#### Key Fields

| Field | Type | Description |
|-------|------|-------------|
| `specialty` | `string` | Matches Clinic specialty |
| `available` | `bool` | False while treating a patient |
| `workingHours` | `pair<int,int>` | `{startHour, endHour}`, e.g. `{8, 16}` |
| `minutesUntilFree` | `int` | Countdown; 0 when available |
| `patientsServedToday` | `int` | Statistics counter |

#### Key Methods

| Method | Description |
|--------|-------------|
| `assignPatient(int minutes)` | Marks doctor busy; sets `minutesUntilFree`; increments served count |
| `tick()` | Decrements `minutesUntilFree`; sets `available = true` when it reaches 0 |
| `isAvailable()` | Returns true if doctor is free |
| `isOnDuty(int hour)` | Returns true if `hour >= start && hour < end` |

---

### PriorityQueue

**File:** `PriorityQueue.h`

A max-heap of `Patient*` pointers, ordered by `Patient::getPriorityScore()`. The key design choice is a `HashMap positionMap` that maps each patient's ID (as a string key) to their current index in the heap array. This enables O(log n) priority updates — without it, finding a specific patient to update would cost O(n).

#### Internal Heap Operations

| Operation | Complexity | Description |
|-----------|------------|-------------|
| `heapifyUp(idx)` | O(log n) | Moves a node upward until heap property is restored |
| `heapifyDown(idx)` | O(log n) | Moves a node downward until heap property is restored |
| `swap(i, j)` | O(1) | Swaps two nodes and updates both positionMap entries atomically |

Every `swap` call updates the `positionMap` for both swapped nodes immediately, keeping the map perfectly consistent with the heap array at all times.

#### Public Interface

| Method | Complexity | Description |
|--------|------------|-------------|
| `insert(Patient*)` | O(log n) | Inserts a patient; resizes if needed; calls `heapifyUp` |
| `extractMax()` | O(log n) | Removes and returns the highest-priority patient |
| `updatePriority(int id)` | O(log n) | Looks up the patient's index in positionMap, calls `heapifyUp` then `heapifyDown` |
| `applyAgingAndRebuild()` | O(n log n) | Ages all patients, then rebuilds heap bottom-up in O(n) |
| `peek()` | O(1) | Returns top patient without removing |
| `isEmpty()` | O(1) | Returns true if heap is empty |
| `getSize()` | O(1) | Returns number of patients in heap |

#### Why HashMap as positionMap?

Without a position map, calling `updatePriority(patientId)` would require scanning the entire heap array to find the patient — O(n). With the HashMap, we get the heap index in O(1), making the full update O(log n).

---

### Clinic

**File:** `Clinic.h`

Represents one hospital department (e.g. Emergency, Cardiology, General). Each Clinic owns its own `PriorityQueue`, list of doctors, and a preset condition menu shown to patients on login.

This is the system's **multi-level priority** implementation: patients first compete to get into the right clinic, then compete within their clinic's own priority queue.

#### Key Fields

| Field | Type | Description |
|-------|------|-------------|
| `patientQueue` | `PriorityQueue` | Per-clinic max-heap |
| `doctors` | `vector<Doctor*>` | Doctors assigned to this department |
| `commonConditions` | `vector<Condition>` | Preset menu for patient selection |

#### Key Methods

| Method | Description |
|--------|-------------|
| `addCommonCondition(Condition)` | Adds a condition to the clinic's preset menu |
| `getConditionByIndex(int)` | Returns preset condition by index; nullptr if "Other" or out of range |
| `displayConditionMenu()` | Prints the numbered condition list to the console |
| `addDoctor(Doctor*)` | Registers a doctor to this clinic |
| `getAvailableDoctor(int hour)` | Scans doctor list; returns first available doctor on duty |
| `enqueuePatient(Patient*)` | Inserts patient into the clinic's priority queue |
| `dequeuePatient()` | Extracts and returns the highest-priority patient |
| `peekNextPatient()` | Returns top patient without removing |
| `applyAgingTick()` | Calls `PriorityQueue::applyAgingAndRebuild()` — called each simulation tick |
| `recordServedPatient(wait, treatment)` | Updates running statistics |
| `getAverageWaitTime()` | Returns total wait / patients served |
| `getThroughput()` | Returns patients served per hour equivalent |

---

### Receptionist

**File:** `Receptionist.h`

Inherits from `User`. Manages two responsibilities:

1. **FIFO pending queue** — patients who selected "Other" are enqueued in arrival order using an internal linked list. This is a plain FIFO queue, not a priority queue.
2. **Triage** — `assignUrgencyAndRoute()` dequeues the next pending patient, assigns a `Category` and estimated treatment time, and routes them into the correct clinic's priority queue.

#### Why a Separate FIFO Queue?

The Receptionist's pending queue deliberately uses a simple linked-list FIFO, not the priority max-heap. This is architecturally intentional — a patient who has not yet been assessed cannot have a priority score. The FIFO represents the waiting-room experience: you sit and wait in order of arrival until the receptionist calls you.

Once triage is complete, the patient enters the competitive priority queue.

#### Key Methods

| Method | Description |
|--------|-------------|
| `receivePendingPatient(Patient*)` | Enqueues patient at rear of FIFO linked list — O(1) |
| `peekNextPending()` | Returns front of FIFO without removing — O(1) |
| `assignUrgencyAndRoute(level, minutes, Clinic*)` | Dequeues front patient, sets condition, routes to clinic |
| `getPendingCount()` | Returns number of patients awaiting triage |
| `hasPending()` | Returns true if any patients need triage |
| `setOnDuty(bool)` | Toggles receptionist duty status |

---

### PriorityQueueManager

**File:** `PriorityQueueManager.h`

The central orchestrator. Owns references to all Clinics and the Receptionist. Drives the simulation loop, handles patient routing, computes system-wide statistics, and runs the FIFO vs Priority comparison.

#### Simulation Tick

Each call to `tick()` performs the following steps in order:

```
1. Increment tick counter
2. For each Doctor in every Clinic: call doctor.tick()
   → decrements minutesUntilFree, frees doctor if countdown hits 0
3. For each Clinic:
   → while clinic has waiting patients AND an available doctor exists:
       dequeue highest-priority patient
       call patient.markServed(currentTick)
       call doctor.assignPatient(condition.estimatedTreatmentMinutes)
       record statistics in clinic
4. For each Clinic: call clinic.applyAgingTick()
   → all still-waiting patients get their aging counter incremented
   → priority scores recomputed
   → heap rebuilt to reflect new ordering
```

#### Key Methods

| Method | Description |
|--------|-------------|
| `addClinic(Clinic*)` | Registers a clinic with the manager |
| `setReceptionist(Receptionist*)` | Assigns the receptionist |
| `getClinicById(int)` | Looks up a clinic by ID; returns nullptr if not found |
| `registerPatient(Patient*, Condition)` | Sets condition, enqueues directly into clinic's priority queue |
| `registerPendingPatient(Patient*)` | Sends patient to receptionist's FIFO queue |
| `tick()` | Advances the simulation by one tick (see above) |
| `setCurrentHour(int)` | Sets simulated hour for doctor shift checks |
| `printSystemStats()` | Prints per-clinic stats and per-patient wait times |
| `compareFifoVsPriority()` | Simulates FIFO ordering on the same patients; compares avg wait |

---

## Data Flow

```
Patient logs in
│
├── Chooses Clinic
│     │
│     ├── Picks preset Condition
│     │       └── setCondition() called → priority computed
│     │           PriorityQueueManager.registerPatient()
│     │               └── Clinic.enqueuePatient() → PriorityQueue.insert()
│     │
│     └── Picks "Other"
│             └── PriorityQueueManager.registerPendingPatient()
│                     └── Receptionist.receivePendingPatient()
│                             → added to FIFO linked-list queue
│
│                             [Later: Receptionist triages]
│                             Receptionist.assignUrgencyAndRoute()
│                                 └── setCondition() → priority computed
│                                     Clinic.enqueuePatient() → PriorityQueue.insert()
│
Simulation tick()
│
├── All Doctors: tick() → count down minutesUntilFree
│
├── Each Clinic:
│     └── while queue not empty AND doctor available:
│           Patient* p = dequeuePatient()      → PriorityQueue.extractMax()
│           p.markServed(currentTick)
│           doctor.assignPatient(p.condition.estimatedTreatmentMinutes)
│           clinic.recordServedPatient(wait, treatment)
│
└── Each Clinic: applyAgingTick()
      └── PriorityQueue.applyAgingAndRebuild()
            → all waiting patients: incrementAging() → score boosted
            → heap rebuilt bottom-up O(n)
```

---

## Priority Formula

The priority score for each patient is computed as:

```
priorityScore = (urgencyLevel × 1,000,000) - arrivalTime + (agingCounter × 100)
```

This encodes three rules with explicit dominance:

### Rule 1 — Urgency always wins
The `urgencyLevel × 1,000,000` term dominates all others. A Critical patient (urgency 5) has a base score of 5,000,000. An Urgent patient (urgency 4) has a base score of 4,000,000. No amount of arrival time or aging can ever overcome a one-level urgency difference.

### Rule 2 — FIFO tie-break for equal urgency
Among patients with the same urgency, the `- arrivalTime` term means earlier arrivals have a higher score. Arrival time 0 gives a larger score than arrival time 10.

### Rule 3 — Aging prevents starvation
Each simulation tick, `agingCounter` increments by 1, adding 100 to the score. A low-urgency patient waiting for a very long time will eventually overtake a slightly higher-urgency patient who arrived much later. This prevents the case where Elective patients wait forever because Urgent patients keep arriving.

### Example

| Patient | Urgency | arrivalTime | agingCounter | Score |
|---------|---------|-------------|--------------|-------|
| Omar    | Critical (5) | 0 | 0 | 5,000,000 |
| Layla   | Urgent (4)   | 0 | 0 | 4,000,000 |
| Ali     | Urgent (4)   | 5 | 0 | 3,999,995 |
| Sara    | Elective (1) | 0 | 10,001 | 2,000,100 |

Omar is always served first. Layla beats Ali because they share urgency but Layla arrived earlier. Sara has been waiting long enough to overtake Layla with 10,001 aging ticks — starvation prevention in action.

---

## Complexity Analysis

| Operation | Complexity | Implementation |
|-----------|------------|----------------|
| Insert patient | O(log n) | `heapifyUp` after append |
| Remove highest-priority | O(log n) | Swap root with last, `heapifyDown` |
| Update single patient priority | O(log n) | `positionMap` lookup O(1), `heapifyUp` + `heapifyDown` O(log n) |
| Apply aging (all patients) | O(n log n) | Increment all O(n), rebuild heap O(n) via bottom-up heapify |
| Enqueue pending (Receptionist) | O(1) | Linked-list rear append |
| Triage next pending patient | O(log n) | O(1) dequeue from FIFO + O(log n) insert into clinic PQ |
| Compute statistics | O(n) | Single pass over patient list |
| FIFO vs Priority comparison | O(n log n) | Sort by arrival time, then single pass |

---

## Challenge Section

The project specification lists four challenge items. All four are addressed:

### 1. Multi-level priority handling
Patients are first routed to the correct **Clinic** (emergency, cardiology, general, etc.), then compete within that clinic's own `PriorityQueue`. This is multi-level: the clinic assignment is the first level, the urgency-based heap is the second.

### 2. Real-time entity arrivals
`PriorityQueueManager::tick()` drives the simulation. New patients can be registered at any tick by calling `registerPatient()` or `registerPendingPatient()` before or after `tick()`. Patients arriving mid-simulation are inserted into a live heap and immediately compete with those already waiting.

### 3. FIFO vs Priority comparison
`PriorityQueueManager::compareFifoVsPriority()` takes the same set of patients, sorts them by arrival time to simulate pure FIFO ordering, computes what the average wait time would have been, and compares it against the actual priority-based average wait. The two queues are also live in the system simultaneously — the Receptionist's desk is FIFO, the Clinic is priority-based.

### 4. System throughput analysis
Each `Clinic` tracks `totalThroughputMinutes` (sum of treatment times) and `totalPatientsServed`. `getThroughput()` converts these into a patients-per-hour rate. `PriorityQueueManager::printSystemStats()` displays per-clinic throughput alongside average wait times.

---

## How to Compile and Run

### Requirements
- C++17 or later
- `HASHMAP.h` must be in the same directory (provided by teammates)
- The `static const float MAX_LOAD` line in `HASHMAP.h` must be changed to `static constexpr float MAX_LOAD` for C++17 compliance

### Demo
```bash
g++ -std=c++17 -o HospitalDemo HospitalDemo.cpp Patient.cpp
./HospitalDemo
```

### Test Suite
```bash
g++ -std=c++17 -o main_test main_test.cpp Patient.cpp
./main_test
```

Expected output ends with:
```
==========================================
  ALL TESTS PASSED (171/171)
==========================================
```

### All files together
```bash
g++ -std=c++17 -Wall -o main_test main_test.cpp Patient.cpp && ./main_test
```
