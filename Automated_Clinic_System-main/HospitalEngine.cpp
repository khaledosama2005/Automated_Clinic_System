#include "HospitalEngine.h"

#include <algorithm>
#include <fstream>
#include <sstream>

using SimpleJson::Value;
using SimpleJson::Object;
using SimpleJson::Array;

static std::string readAllText(const std::string& path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static bool writeAllText(const std::string& path, const std::string& data) {
    std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << data;
    return (bool)out;
}

HospitalEngine::HospitalEngine(std::string statePath)
    : statePath(std::move(statePath)),
      nextPatientId(1),
      nextClinicId(4),
      arrivalSeq(0)
{
    pendingIds.clear();
    ensureDefaultSetup();
}

Clinic* HospitalEngine::findClinic(int clinicId) const {
    for (auto& c : clinics) {
        if (c && c->getId() == clinicId) return c.get();
    }
    return nullptr;
}

void HospitalEngine::ensureDefaultSetup() {
    if (!clinics.empty()) return;

    clinics.emplace_back(std::make_unique<Clinic>(1, "Emergency", "General / ER"));
    clinics.emplace_back(std::make_unique<Clinic>(2, "Cardiology", "Heart"));
    clinics.emplace_back(std::make_unique<Clinic>(3, "Pediatrics", "Children"));

    // Preset conditions (examples)
    clinics[0]->addCommonCondition(Condition("Severe bleeding", UrgencyLevel::CRITICAL, 45));
    clinics[0]->addCommonCondition(Condition("High fever", UrgencyLevel::URGENT, 20));
    clinics[0]->addCommonCondition(Condition("Minor cut", UrgencyLevel::NON_URGENT, 10));

    clinics[1]->addCommonCondition(Condition("Chest pain", UrgencyLevel::CRITICAL, 60));
    clinics[1]->addCommonCondition(Condition("Palpitations", UrgencyLevel::URGENT, 30));
    clinics[1]->addCommonCondition(Condition("Routine check", UrgencyLevel::ELECTIVE, 15));

    clinics[2]->addCommonCondition(Condition("Breathing difficulty", UrgencyLevel::URGENT, 25));
    clinics[2]->addCommonCondition(Condition("Ear infection", UrgencyLevel::NON_URGENT, 15));
}

bool HospitalEngine::load(std::string& error) {
    ensureDefaultSetup();
    std::string txt = readAllText(statePath);
    if (txt.empty()) return true; // no state yet

    auto parsed = SimpleJson::parse(txt);
    if (!parsed.ok) {
        error = "state.json parse failed: " + parsed.error;
        // If the state file is corrupt, fall back to a clean slate.
        std::string e;
        reset(e);
        error.clear();
        return true;
    }
    if (!fromJson(parsed.value, error)) {
        // Backward-compat: older versions had a different state shape.
        // If we can't load, start fresh so the system remains usable.
        std::string e;
        reset(e);
        error.clear();
        return true;
    }
    return true;
}

bool HospitalEngine::save(std::string& error) const {
    (void)error;
    Value root = toJson();
    if (!writeAllText(statePath, SimpleJson::stringify(root))) {
        error = "Could not write state file: " + statePath;
        return false;
    }
    return true;
}

bool HospitalEngine::reset(std::string& error) {
    (void)error;
    nextPatientId = 1;
    nextClinicId = 4;
    arrivalSeq = 0;
    patients.clear();
    clinics.clear();
    pendingIds.clear();
    ensureDefaultSetup();
    return save(error);
}

Value HospitalEngine::ok(Value data) {
    return SimpleJson::object({
        {"status", "ok"},
        {"data", std::move(data)}
    });
}

Value HospitalEngine::err(const std::string& message) {
    return SimpleJson::object({
        {"status", "error"},
        {"message", message}
    });
}

Value HospitalEngine::handle(const std::string& action, const Value& data) {
    if (action == "get_clinics") return ok(api_get_clinics());
    if (action == "get_state") return ok(api_get_state());
    // Backward-compat with older UI/bridge naming
    if (action == "get_queue_state") return ok(api_get_state());
    if (action == "get_stats") return ok(api_get_state());
    if (action == "add_patient") return api_add_patient(data);
    if (action == "add_pending_patient") return api_add_pending_patient(data);
    if (action == "admin_add_clinic") return api_admin_add_clinic(data);
    if (action == "admin_remove_clinic") return api_admin_remove_clinic(data);
    if (action == "admin_add_symptom") return api_admin_add_symptom(data);
    if (action == "admin_remove_symptom") return api_admin_remove_symptom(data);
    if (action == "admin_finish_next") return api_admin_finish_next(data);
    if (action == "admin_triage_pending") return api_admin_triage_pending(data);
    if (action == "reset") {
        std::string e;
        if (!reset(e)) return err(e.empty() ? "reset failed" : e);
        return ok(SimpleJson::object({{"message", "reset ok"}}));
    }
    return err("Unknown action: " + action);
}

// ---------------- API: read-only ----------------

Value HospitalEngine::api_get_clinics() const {
    Array arr;
    for (const auto& c : clinics) {
        if (!c) continue;
        Array conds;
        const auto& cc = c->getCommonConditions();
        for (int i = 0; i < (int)cc.size(); ++i) {
            conds.push_back(SimpleJson::object({
                {"index", i},
                {"name", cc[i].name},
                {"urgency", (int)cc[i].category.level},
                {"urgencyName", cc[i].category.name},
                {"estimatedTreatmentMinutes", cc[i].estimatedTreatmentMinutes}
            }));
        }
        arr.push_back(SimpleJson::object({
            {"id", c->getId()},
            {"name", c->getName()},
            {"specialty", c->getSpecialty()},
            {"conditions", std::move(conds)}
        }));
    }
    return Value(std::move(arr));
}

Value HospitalEngine::api_get_state() const {
    Array clinicStates;
    for (const auto& c : clinics) {
        if (!c) continue;
        Array waiting;
        for (Patient* p : c->getQueueItems()) {
            if (!p) continue;
            waiting.push_back(SimpleJson::object({
                {"id", p->getId()},
                {"name", p->getName()},
                {"phone", p->getPhone()},
                {"priorityScore", p->getPriorityScore()},
                {"arrivalSeq", p->getArrivalTime()},
                {"symptom", p->getCondition().name},
                {"urgency", (int)p->getCondition().category.level},
                {"urgencyName", p->getCondition().category.name},
                {"estimatedTreatmentMinutes", p->getCondition().estimatedTreatmentMinutes}
            }));
        }
        clinicStates.push_back(SimpleJson::object({
            {"clinicId", c->getId()},
            {"clinicName", c->getName()},
            {"specialty", c->getSpecialty()},
            {"queueSize", c->getQueueSize()},
            {"waiting", std::move(waiting)}
        }));
    }

    Array pending;
    for (int pid : pendingIds) {
        auto it = patients.find(pid);
        if (it == patients.end() || !it->second) continue;
        Patient* p = it->second.get();
        pending.push_back(SimpleJson::object({
            {"id", p->getId()},
            {"name", p->getName()},
            {"phone", p->getPhone()},
            {"clinicId", p->getClinicId()},
            {"reportedSymptoms", p->getReportedSymptoms()},
        }));
    }

    return SimpleJson::object({
        {"clinics", std::move(clinicStates)},
        {"pendingCount", (int)pendingIds.size()},
        {"pending", std::move(pending)}
    });
}

// ---------------- API: mutations ----------------

Value HospitalEngine::api_add_patient(const Value& data) {
    const Value* nameV = data.get("name");
    const Value* clinicV = data.get("clinicId");
    // Backward-compat: older UIs used conditionIndex
    const Value* symptomIdxV = data.get("symptomIndex");
    if (!symptomIdxV) symptomIdxV = data.get("conditionIndex");
    const Value* phoneV = data.get("phone");
    if (!nameV || !clinicV || !symptomIdxV || !nameV->isString()) {
        return err("add_patient requires {name, phone?, clinicId, symptomIndex}");
    }

    int clinicId = clinicV->asInt(-1);
    int symptomIdx  = symptomIdxV->asInt(-1);
    Clinic* clinic = findClinic(clinicId);
    if (!clinic) return err("Invalid clinicId");

    const Condition* preset = clinic->getConditionByIndex(symptomIdx);
    if (!preset) return err("Invalid symptomIndex");

    int id = nextPatientId++;
    std::string phone = (phoneV && phoneV->isString()) ? phoneV->asString() : "";
    auto p = std::make_unique<Patient>(id, nameV->asString(), phone, arrivalSeq++, clinicId);
    p->setCondition(*preset);

    Patient* raw = p.get();
    patients.emplace(id, std::move(p));
    clinic->enqueuePatient(raw);

    std::string e;
    if (!save(e)) return err(e);

    return ok(SimpleJson::object({
        {"patientId", id},
        {"queued", true},
        {"clinicId", clinicId},
        {"priorityScore", raw->getPriorityScore()}
    }));
}

Value HospitalEngine::api_add_pending_patient(const Value& data) {
    const Value* nameV = data.get("name");
    const Value* clinicV = data.get("clinicId");
    const Value* phoneV = data.get("phone");
    const Value* symptomsV = data.get("symptomsText");

    if (!nameV || !clinicV || !nameV->isString()) {
        return err("add_pending_patient requires {name, phone?, clinicId, symptomsText}");
    }
    if (!symptomsV || !symptomsV->isString() || symptomsV->asString().empty()) {
        return err("symptomsText is required");
    }

    int clinicId = clinicV->asInt(-1);
    Clinic* clinic = findClinic(clinicId);
    if (!clinic) return err("Invalid clinicId");

    int id = nextPatientId++;
    std::string phone = (phoneV && phoneV->isString()) ? phoneV->asString() : "";
    auto p = std::make_unique<Patient>(id, nameV->asString(), phone, arrivalSeq++, clinicId);
    p->setReportedSymptoms(symptomsV->asString());
    // pending condition until admin triage
    p->setCondition(Condition("Other (pending triage)", UrgencyLevel::PENDING, 0));

    patients.emplace(id, std::move(p));
    pendingIds.push_back(id);

    std::string e;
    if (!save(e)) return err(e);

    return ok(SimpleJson::object({
        {"patientId", id},
        {"pending", true},
        {"clinicId", clinicId}
    }));
}

Value HospitalEngine::api_admin_add_clinic(const Value& data) {
    const Value* nameV = data.get("name");
    const Value* specV = data.get("specialty");
    if (!nameV || !nameV->isString()) return err("admin_add_clinic requires {name, specialty?}");
    std::string spec = (specV && specV->isString()) ? specV->asString() : "General";
    int id = nextClinicId++;
    clinics.emplace_back(std::make_unique<Clinic>(id, nameV->asString(), spec));
    std::string e;
    if (!save(e)) return err(e);
    return ok(SimpleJson::object({{"clinicId", id}}));
}

Value HospitalEngine::api_admin_remove_clinic(const Value& data) {
    const Value* clinicV = data.get("clinicId");
    if (!clinicV) return err("admin_remove_clinic requires {clinicId}");
    int id = clinicV->asInt(-1);
    for (size_t i = 0; i < clinics.size(); ++i) {
        if (clinics[i] && clinics[i]->getId() == id) {
            clinics.erase(clinics.begin() + (long long)i);
            std::string e;
            if (!save(e)) return err(e);
            return ok(SimpleJson::object({{"removed", true}}));
        }
    }
    return err("clinicId not found");
}

Value HospitalEngine::api_admin_add_symptom(const Value& data) {
    const Value* clinicV = data.get("clinicId");
    const Value* nameV = data.get("name");
    const Value* urgV = data.get("urgencyLevel");
    const Value* estV = data.get("estimatedTreatmentMinutes");
    if (!clinicV || !nameV || !urgV || !estV || !nameV->isString()) {
        return err("admin_add_symptom requires {clinicId, name, urgencyLevel(1..5), estimatedTreatmentMinutes}");
    }
    Clinic* c = findClinic(clinicV->asInt(-1));
    if (!c) return err("Invalid clinicId");
    int u = urgV->asInt(0);
    if (u < 1 || u > 5) return err("urgencyLevel must be 1..5");
    int est = estV->asInt(0);
    if (est <= 0) return err("estimatedTreatmentMinutes must be > 0");
    c->addCommonCondition(Condition(nameV->asString(), (UrgencyLevel)u, est));
    std::string e;
    if (!save(e)) return err(e);
    return ok(SimpleJson::object({{"added", true}}));
}

Value HospitalEngine::api_admin_remove_symptom(const Value& data) {
    const Value* clinicV = data.get("clinicId");
    const Value* idxV = data.get("symptomIndex");
    if (!clinicV || !idxV) return err("admin_remove_symptom requires {clinicId, symptomIndex}");
    Clinic* c = findClinic(clinicV->asInt(-1));
    if (!c) return err("Invalid clinicId");
    if (!c->removeConditionByIndex(idxV->asInt(-1))) return err("Invalid symptomIndex");
    std::string e;
    if (!save(e)) return err(e);
    return ok(SimpleJson::object({{"removed", true}}));
}

Value HospitalEngine::api_admin_finish_next(const Value& data) {
    const Value* clinicV = data.get("clinicId");
    if (!clinicV) return err("admin_finish_next requires {clinicId}");
    Clinic* c = findClinic(clinicV->asInt(-1));
    if (!c) return err("Invalid clinicId");
    Patient* p = c->dequeuePatient();
    if (!p) return err("Queue empty");
    p->markServed(p->getArrivalTime());
    std::string e;
    if (!save(e)) return err(e);
    return ok(SimpleJson::object({{"finishedPatientId", p->getId()}}));
}

Value HospitalEngine::api_admin_triage_pending(const Value& data) {
    const Value* urgencyV = data.get("urgencyLevel");
    const Value* estV = data.get("estimatedTreatmentMinutes");
    const Value* clinicV = data.get("clinicId");
    if (!urgencyV || !estV || !clinicV) {
        return err("admin_triage_pending requires {clinicId, urgencyLevel(1..5), estimatedTreatmentMinutes}");
    }
    if (pendingIds.empty()) return err("No pending patients");

    int clinicId = clinicV->asInt(-1);
    Clinic* clinic = findClinic(clinicId);
    if (!clinic) return err("Invalid clinicId");

    int u = urgencyV->asInt(0);
    if (u < 1 || u > 5) return err("urgencyLevel must be 1..5");
    int est = estV->asInt(0);
    if (est <= 0) return err("estimatedTreatmentMinutes must be > 0");

    int pid = pendingIds.front();
    pendingIds.erase(pendingIds.begin());
    auto it = patients.find(pid);
    if (it == patients.end()) return err("Pending patient not found");

    Patient* p = it->second.get();
    // Use the patient's reported text as the condition name
    std::string symptomName = p->getReportedSymptoms().empty() ? "Other" : p->getReportedSymptoms();
    p->setCondition(Condition(symptomName, (UrgencyLevel)u, est));
    clinic->enqueuePatient(p);

    std::string e;
    if (!save(e)) return err(e);

    return ok(SimpleJson::object({
        {"patientId", pid},
        {"triaged", true},
        {"clinicId", clinicId},
        {"priorityScore", p->getPriorityScore()}
    }));
}

// ---------------- Serialization ----------------

Value HospitalEngine::toJson() const {
    Array patientsArr;
    patientsArr.reserve(patients.size());
    for (const auto& kv : patients) {
        const Patient* p = kv.second.get();
        if (!p) continue;
        patientsArr.push_back(SimpleJson::object({
            {"id", p->getId()},
            {"name", p->getName()},
            {"phone", p->getPhone()},
            {"reportedSymptoms", p->getReportedSymptoms()},
            {"clinicId", p->getClinicId()},
            {"arrivalTime", p->getArrivalTime()},
            {"agingCounter", p->getAgingCounter()},
            {"served", p->isServed()},
            {"serviceStartTime", p->getServiceStartTime()},
            {"conditionName", p->getCondition().name},
            {"urgencyLevel", (int)p->getCondition().category.level},
            {"estimatedTreatmentMinutes", p->getCondition().estimatedTreatmentMinutes}
        }));
    }

    Array clinicQueues;
    for (const auto& c : clinics) {
        if (!c) continue;
        Array ids;
        for (Patient* p : c->getQueueItems()) {
            if (p) ids.push_back(p->getId());
        }
        clinicQueues.push_back(SimpleJson::object({
            {"clinicId", c->getId()},
            {"waitingIds", std::move(ids)}
        }));
    }

    Array clinicsArr;
    for (const auto& c : clinics) {
        if (!c) continue;
        Array symptoms;
        const auto& cc = c->getCommonConditions();
        for (int i = 0; i < (int)cc.size(); ++i) {
            symptoms.push_back(SimpleJson::object({
                {"name", cc[i].name},
                {"urgencyLevel", (int)cc[i].category.level},
                {"estimatedTreatmentMinutes", cc[i].estimatedTreatmentMinutes}
            }));
        }
        clinicsArr.push_back(SimpleJson::object({
            {"id", c->getId()},
            {"name", c->getName()},
            {"specialty", c->getSpecialty()},
            {"symptoms", std::move(symptoms)}
        }));
    }

    return SimpleJson::object({
        {"nextPatientId", nextPatientId},
        {"nextClinicId", nextClinicId},
        {"arrivalSeq", arrivalSeq},
        {"patients", std::move(patientsArr)},
        {"clinics", std::move(clinicsArr)},
        {"clinicQueues", std::move(clinicQueues)},
        {"pendingIds", [&]() {
            Array a;
            a.reserve(pendingIds.size());
            for (int pid : pendingIds) a.push_back(pid);
            return Value(std::move(a));
        }()}
    });
}

bool HospitalEngine::fromJson(const Value& root, std::string& error) {
    if (!root.isObject()) { error = "state root is not an object"; return false; }
    const Value* nextV = root.get("nextPatientId");
    const Value* nextClinicV = root.get("nextClinicId");
    const Value* arrivalV = root.get("arrivalSeq");
    const Value* patsV = root.get("patients");
    const Value* clinicsV = root.get("clinics");
    if (!nextV || !patsV || !patsV->isArray() || !clinicsV || !clinicsV->isArray()) {
        error = "state missing fields";
        return false;
    }

    nextPatientId = nextV->asInt(1);
    nextClinicId = nextClinicV ? nextClinicV->asInt(4) : 4;
    arrivalSeq = arrivalV ? arrivalV->asInt(0) : 0;

    patients.clear();
    clinics.clear();
    pendingIds.clear();

    // Recreate clinics + symptoms
    for (const Value& cv : clinicsV->asArray()) {
        if (!cv.isObject()) continue;
        int id = cv.get("id") ? cv.get("id")->asInt(-1) : -1;
        std::string name = (cv.get("name") && cv.get("name")->isString()) ? cv.get("name")->asString() : "";
        std::string spec = (cv.get("specialty") && cv.get("specialty")->isString()) ? cv.get("specialty")->asString() : "General";
        if (id < 0 || name.empty()) continue;
        auto clinic = std::make_unique<Clinic>(id, name, spec);
        const Value* symV = cv.get("symptoms");
        if (symV && symV->isArray()) {
            for (const Value& sv : symV->asArray()) {
                if (!sv.isObject()) continue;
                std::string sname = (sv.get("name") && sv.get("name")->isString()) ? sv.get("name")->asString() : "";
                int urg = sv.get("urgencyLevel") ? sv.get("urgencyLevel")->asInt(1) : 1;
                int est = sv.get("estimatedTreatmentMinutes") ? sv.get("estimatedTreatmentMinutes")->asInt(10) : 10;
                if (!sname.empty()) clinic->addCommonCondition(Condition(sname, (UrgencyLevel)urg, est));
            }
        }
        clinics.emplace_back(std::move(clinic));
    }
    if (clinics.empty()) ensureDefaultSetup();

    // Recreate patients
    for (const Value& pv : patsV->asArray()) {
        if (!pv.isObject()) continue;
        int id = pv.get("id") ? pv.get("id")->asInt(-1) : -1;
        std::string name = (pv.get("name") && pv.get("name")->isString()) ? pv.get("name")->asString() : "";
        std::string phone = (pv.get("phone") && pv.get("phone")->isString()) ? pv.get("phone")->asString() : "";
        std::string reported = (pv.get("reportedSymptoms") && pv.get("reportedSymptoms")->isString()) ? pv.get("reportedSymptoms")->asString() : "";
        int clinicId = pv.get("clinicId") ? pv.get("clinicId")->asInt(1) : 1;
        int arrival = pv.get("arrivalTime") ? pv.get("arrivalTime")->asInt(0) : 0;
        int aging = pv.get("agingCounter") ? pv.get("agingCounter")->asInt(0) : 0;
        bool served = pv.get("served") ? pv.get("served")->asBool(false) : false;
        int serviceStart = pv.get("serviceStartTime") ? pv.get("serviceStartTime")->asInt(-1) : -1;

        std::string cName = (pv.get("conditionName") && pv.get("conditionName")->isString()) ? pv.get("conditionName")->asString() : "Other";
        int urgency = pv.get("urgencyLevel") ? pv.get("urgencyLevel")->asInt(0) : 0;
        int est = pv.get("estimatedTreatmentMinutes") ? pv.get("estimatedTreatmentMinutes")->asInt(0) : 0;

        if (id < 0 || name.empty()) continue;
        auto p = std::make_unique<Patient>(id, name, phone, arrival, clinicId);
        p->setReportedSymptoms(reported);
        p->setCondition(Condition(cName, (UrgencyLevel)urgency, est));
        p->setAgingCounterForLoad(aging);
        if (served && serviceStart >= 0) p->setServiceStateForLoad(true, serviceStart);

        patients.emplace(id, std::move(p));
    }

    // Recreate clinics/queues from ids
    const Value* cqV = root.get("clinicQueues");
    if (cqV && cqV->isArray()) {
        for (const Value& cq : cqV->asArray()) {
            if (!cq.isObject()) continue;
            int clinicId = cq.get("clinicId") ? cq.get("clinicId")->asInt(-1) : -1;
            Clinic* clinic = findClinic(clinicId);
            if (!clinic) continue;
            const Value* idsV = cq.get("waitingIds");
            if (!idsV || !idsV->isArray()) continue;
            for (const Value& idV : idsV->asArray()) {
                int pid = idV.asInt(-1);
                auto it = patients.find(pid);
                if (it != patients.end()) clinic->enqueuePatient(it->second.get());
            }
        }
    }

    const Value* pendV = root.get("pendingIds");
    if (pendV && pendV->isArray()) {
        for (const Value& idV : pendV->asArray()) {
            int pid = idV.asInt(-1);
            if (pid >= 0) pendingIds.push_back(pid);
        }
    }

    return true;
}

