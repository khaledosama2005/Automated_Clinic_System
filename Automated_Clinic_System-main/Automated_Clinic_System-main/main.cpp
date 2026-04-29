#include <iostream>
#include <string>

#include "HospitalEngine.h"

// hospital_engine entrypoint:
// - reads exactly one JSON object from stdin
// - writes exactly one JSON object to stdout
// - persists to --state <path>
int main(int argc, char** argv) {
    std::string statePath = "state.json";
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--state" && i + 1 < argc) {
            statePath = argv[++i];
        }
    }

    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) {
        std::cout << SimpleJson::stringify(SimpleJson::object({
            {"status", "error"},
            {"message", "No input provided to stdin"}
        }));
        return 0;
    }

    auto parsed = SimpleJson::parse(input);
    if (!parsed.ok || !parsed.value.isObject()) {
        std::cout << SimpleJson::stringify(SimpleJson::object({
            {"status", "error"},
            {"message", parsed.ok ? "Input is not a JSON object" : ("Invalid JSON: " + parsed.error)}
        }));
        return 0;
    }

    const SimpleJson::Value* actionV = parsed.value.get("action");
    const SimpleJson::Value* dataV   = parsed.value.get("data");
    if (!actionV || !actionV->isString()) {
        std::cout << SimpleJson::stringify(SimpleJson::object({
            {"status", "error"},
            {"message", "Missing or invalid 'action' field"}
        }));
        return 0;
    }

    HospitalEngine engine(statePath);
    std::string err;
    if (!engine.load(err)) {
        std::cout << SimpleJson::stringify(SimpleJson::object({
            {"status", "error"},
            {"message", err.empty() ? "Failed to load state" : err}
        }));
        return 0;
    }

    SimpleJson::Value data = dataV ? *dataV : SimpleJson::object({});
    SimpleJson::Value resp = engine.handle(actionV->asString(), data);
    std::cout << SimpleJson::stringify(resp);
    return 0;
}