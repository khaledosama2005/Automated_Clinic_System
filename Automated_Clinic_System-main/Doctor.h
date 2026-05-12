#ifndef DOCTOR_H
#define DOCTOR_H

#include "User.h"
#include <utility>

// ============================================================
// Doctor Class
// Each Doctor belongs to one Clinic (by clinicId).
// When a Patient is dequeued and assigned, minutesUntilFree
// is set from the patient's condition.estimatedTreatmentMinutes.
// Each simulation tick(), the countdown decrements until the
// doctor is free again (isAvailable = true).
// ============================================================
class Doctor : public User {
private:
    std::string        specialty;
    bool               available;
    std::pair<int,int> workingHours;       // {startHour, endHour}, e.g. {8, 16}
    int                minutesUntilFree;   // 0 when available
    int                patientsServedToday;
    int                assignedClinicId;

public:
    Doctor(int id,
           const std::string& name,
           const std::string& specialty,
           int clinicId,
           std::pair<int,int> hours = {8, 16})
        : User(id, name, UserRole::DOCTOR),
          specialty(specialty),
          available(true),
          workingHours(hours),
          minutesUntilFree(0),
          patientsServedToday(0),
          assignedClinicId(clinicId)
    {}

    // ---- Getters ------------------------------------------------
    std::string        getSpecialty()        const { return specialty; }
    bool               isAvailable()         const { return available; }
    std::pair<int,int> getWorkingHours()     const { return workingHours; }
    int                getMinutesUntilFree() const { return minutesUntilFree; }
    int                getPatientsServed()   const { return patientsServedToday; }
    int                getClinicId()         const { return assignedClinicId; }

    bool isOnDuty(int currentHour) const {
        return currentHour >= workingHours.first && currentHour < workingHours.second;
    }

    // ---- Patient Assignment -------------------------------------
    // Called by PriorityQueueManager when a patient is assigned.
    // Marks doctor as busy for the estimated treatment duration.
    void assignPatient(int treatmentMinutes) {
        available        = false;
        minutesUntilFree = treatmentMinutes;
        patientsServedToday++;
    }

    // ---- Simulation Tick ----------------------------------------
    // Called each simulation tick. Decrements the countdown and
    // releases the doctor once the current patient is done.
    void tick() {
        if (!available && minutesUntilFree > 0) {
            minutesUntilFree--;
            if (minutesUntilFree == 0) {
                available = true;
                std::cerr << "  [Doctor " << name << "] is now available.\n";
            }
        }
    }

    void displayInfo() const override {
        std::cout << "+-----------------------------------------+\n";
        std::cout << "  [Doctor]\n";
        std::cout << "  ID          : " << id << "\n";
        std::cout << "  Name        : " << name << "\n";
        std::cout << "  Specialty   : " << specialty << "\n";
        std::cout << "  Clinic ID   : " << assignedClinicId << "\n";
        std::cout << "  Available   : " << (available ? "Yes" : "No") << "\n";
        std::cout << "  Shift       : " << workingHours.first << ":00 - "
                  << workingHours.second << ":00\n";
        std::cout << "  Until Free  : " << minutesUntilFree << " min\n";
        std::cout << "  Served Today: " << patientsServedToday << "\n";
        std::cout << "+-----------------------------------------+\n";
    }
};

#endif
