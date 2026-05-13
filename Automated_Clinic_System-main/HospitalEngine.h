#ifndef HOSPITALENGINE_H
#define HOSPITALENGINE_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Clinic.h"
#include "SimpleJson.h"
#include "Trie.h"                          // O(m) prefix search for patient names

// ============================================================
// HospitalEngine
// The backend "brain" that the Django bridge talks to.
//
// - Each request launches the executable once.
// - Engine loads state from a file, applies one action, saves state.
// - Replies with exactly one JSON object to stdout.
// ============================================================
class HospitalEngine {
public:
    explicit HospitalEngine(std::string statePath);

    // Load state from disk if present (otherwise create default state).
    bool load(std::string& error);
    bool save(std::string& error) const;
    bool reset(std::string& error);

    // Execute one action request and return a response object:
    // { "status": "ok", "data": ... } or { "status": "error", "message": ... }
    SimpleJson::Value handle(const std::string& action, const SimpleJson::Value& data);

private:
    std::string statePath;

    int nextPatientId;
    int nextClinicId;
    int arrivalSeq;
    std::vector<int> pendingIds; // FIFO: patients awaiting admin triage ("Other")

    // System actors
    std::vector<std::unique_ptr<Clinic>> clinics;

    // Ownership of all patients (so pointers stay valid across queues)
    std::unordered_map<int, std::unique_ptr<Patient>> patients;

    // Trie for O(m) receptionist name-prefix search.
    // Mirrors the patients map — every live (non-served) patient
    // is inserted here on creation and removed on finish_next.
    Trie patientTrie;

    // Helpers
    Clinic* findClinic(int clinicId) const;
    void ensureDefaultSetup();

    // Serialization
    SimpleJson::Value toJson() const;
    bool fromJson(const SimpleJson::Value& root, std::string& error);

    // API actions
    SimpleJson::Value api_get_clinics() const;
    SimpleJson::Value api_get_state() const; // queues + clinics + symptoms
    SimpleJson::Value api_add_patient(const SimpleJson::Value& data);
    SimpleJson::Value api_add_pending_patient(const SimpleJson::Value& data);
    SimpleJson::Value api_search_patients(const SimpleJson::Value& data) const;

    // Admin actions
    SimpleJson::Value api_admin_add_clinic(const SimpleJson::Value& data);
    SimpleJson::Value api_admin_remove_clinic(const SimpleJson::Value& data);
    SimpleJson::Value api_admin_add_symptom(const SimpleJson::Value& data);
    SimpleJson::Value api_admin_remove_symptom(const SimpleJson::Value& data);
    SimpleJson::Value api_admin_finish_next(const SimpleJson::Value& data);
    SimpleJson::Value api_admin_triage_pending(const SimpleJson::Value& data);

    // Response helpers
    static SimpleJson::Value ok(SimpleJson::Value data = SimpleJson::object({}));
    static SimpleJson::Value err(const std::string& message);
};

#endif
