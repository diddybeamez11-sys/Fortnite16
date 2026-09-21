#include "modules/ModuleManager.h"

namespace eclient_runtime::modules {

ModuleManager& ModuleManager::instance() {
    static ModuleManager manager;
    return manager;
}

ModuleManager::ModuleManager() : modules_{
    {"HUD", "Native in-process status HUD", true, false},
    {"ESP", "Entity renderer integration point; waits for a verified 1.21.80 entity mapping", false, true},
    {"Speed", "Player movement integration point; waits for a verified 1.21.80 movement mapping", false, true},
    {"FullBright", "Gamma integration point; waits for a verified 1.21.80 renderer mapping", false, true},
    {"NoHurtCam", "Camera integration point; waits for a verified 1.21.80 camera mapping", false, true},
} {}

const std::vector<ModuleState>& ModuleManager::all() const { return modules_; }

bool ModuleManager::setEnabled(const std::string& name, bool enabled) {
    std::lock_guard lock(mutex_);
    for (auto& module : modules_) {
        if (module.name != name) continue;
        // Do not pretend a UI toggle is an active memory patch. The actual
        // module implementations are enabled only after their exact binary
        // mapping has been validated.
        module.enabled = enabled;
        return true;
    }
    return false;
}

bool ModuleManager::enabled(const std::string& name) const {
    std::lock_guard lock(mutex_);
    for (const auto& module : modules_) if (module.name == name) return module.enabled;
    return false;
}

} // namespace eclient_runtime::modules
