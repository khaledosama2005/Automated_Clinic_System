#ifndef USER_H
#define USER_H

#include <string>
#include <iostream>

// ============================================================
// UserRole Enum
// Distinguishes the three actors in the hospital system.
// ============================================================
enum class UserRole { PATIENT, DOCTOR, RECEPTIONIST };

// ============================================================
// User (Abstract Base Class)
// Provides shared identity fields for all system actors.
// Doctor and Receptionist both inherit from User.
// Patient also inherits from User (login → choose clinic).
// ============================================================
class User {
protected:
    int         id;
    std::string name;
    UserRole    role;

public:
    User(int id, const std::string& name, UserRole role)
        : id(id), name(name), role(role) {}

    virtual ~User() {}

    int         getId()   const { return id; }
    std::string getName() const { return name; }
    UserRole    getRole() const { return role; }

    std::string getRoleString() const {
        switch (role) {
            case UserRole::PATIENT:       return "Patient";
            case UserRole::DOCTOR:        return "Doctor";
            case UserRole::RECEPTIONIST:  return "Receptionist";
        }
        return "Unknown";
    }

    virtual void displayInfo() const = 0;
};

#endif
