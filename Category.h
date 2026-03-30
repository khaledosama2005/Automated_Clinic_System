#ifndef CATEGORY_H
#define CATEGORY_H

#include <string>

// ============================================================
// UrgencyLevel Enum
// Represents the 5-tier triage scale used system-wide.
// PENDING means a patient picked "Other" and awaits receptionist
// assessment before entering the priority queue.
// ============================================================
enum class UrgencyLevel {
    PENDING    = 0,
    ELECTIVE   = 1,
    NON_URGENT = 2,
    SEMI_URGENT = 3,
    URGENT     = 4,
    CRITICAL   = 5
};

// ============================================================
// Category Struct
// Wraps UrgencyLevel with human-readable metadata.
// Used by Condition to encode preset urgency, and by
// Receptionist when manually triaging "Other" patients.
// ============================================================
struct Category {
    UrgencyLevel level;
    std::string  name;
    int          maxWaitMinutes; // threshold before escalation

    explicit Category(UrgencyLevel lvl = UrgencyLevel::PENDING) : level(lvl) {
        switch (lvl) {
            case UrgencyLevel::CRITICAL:     name = "Critical";     maxWaitMinutes = 0;   break;
            case UrgencyLevel::URGENT:       name = "Urgent";       maxWaitMinutes = 30;  break;
            case UrgencyLevel::SEMI_URGENT:  name = "Semi-Urgent";  maxWaitMinutes = 60;  break;
            case UrgencyLevel::NON_URGENT:   name = "Non-Urgent";   maxWaitMinutes = 120; break;
            case UrgencyLevel::ELECTIVE:     name = "Elective";     maxWaitMinutes = 240; break;
            default:                         name = "Pending";      maxWaitMinutes = -1;  break;
        }
    }

    int  getPriorityValue() const { return static_cast<int>(level); }
    bool isPending()        const { return level == UrgencyLevel::PENDING; }
};

#endif
