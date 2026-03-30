#include "Patient.h"
#include <iostream>
#include <iomanip>

void Patient::displayInfo() const {
    std::cout << "+-----------------------------------------+\n";
    std::cout << "  [Patient]\n";
    std::cout << "  ID          : " << id << "\n";
    std::cout << "  Name        : " << name << "\n";
    std::cout << "  Clinic ID   : " << assignedClinicId << "\n";
    std::cout << "  Condition   : " << condition.name << "\n";
    std::cout << "  Urgency     : " << condition.category.name << "\n";
    std::cout << "  Est. Time   : " << condition.estimatedTreatmentMinutes << " min\n";
    std::cout << "  Priority    : " << priorityScore << "\n";
    std::cout << "  Arrived At  : tick " << arrivalTime << "\n";
    std::cout << "  Wait Time   : " << waitTime << " ticks\n";
    std::cout << "  Aging       : " << agingCounter << "\n";
    std::cout << "  Served      : " << (served ? "Yes" : "No") << "\n";
    std::cout << "+-----------------------------------------+\n";
}
