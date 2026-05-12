#ifndef CONDITION_H
#define CONDITION_H

#include "Category.h"
#include <string>

// ============================================================
// Condition Struct
// Represents a medical condition a patient can select.
// Each Clinic holds a vector<Condition> of common presets.
// If a patient picks "Other", a Condition is created with
// UrgencyLevel::PENDING until the Receptionist assigns it.
// estimatedTreatmentMinutes drives Doctor::minutesUntilFree.
// ============================================================
struct Condition {
    std::string name;
    Category    category;
    int         estimatedTreatmentMinutes;

    // Default: "Other" / pending
    Condition()
        : name("Other"),
          category(UrgencyLevel::PENDING),
          estimatedTreatmentMinutes(0) {}

    Condition(const std::string& n, UrgencyLevel lvl, int estTime)
        : name(n),
          category(lvl),
          estimatedTreatmentMinutes(estTime) {}
};

#endif
