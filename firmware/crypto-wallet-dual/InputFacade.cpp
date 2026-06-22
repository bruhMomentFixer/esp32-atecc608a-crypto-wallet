#include "InputFacade.h"

InputFacade::InputFacade(InputManager* jm) :
    joystick(jm), activeMode("TERMINAL+JOY"), lastActivityTime(0) {}

void InputFacade::initialize() {
    // Inicializa componentes si es necesario (ya inicializados en main)
    // Mantener coherencia con tu inicialización actual
}

InputFacade::InputData InputFacade::processInputs() {
    InputFacade::InputData out;
    out.event = InputManager::NO_EVENT;
    out.digit = 0;
    out.menuItem = 0;

    InputManager::InputData data = joystick->processInputs();
    
    out.event = data.event;
    out.digit = data.digit;
    
    if (data.event != InputManager::NO_EVENT) {
        activeMode = (data.event == InputManager::PIN_DIGIT ||
                      data.event == InputManager::NAV_SELECT)
                     ? "TERMINAL"
                     : activeMode;
        lastActivityTime = millis();
    }
    return out;
}
    
void InputFacade::setPinInputMode(bool enabled) {
        joystick->setPinInputMode(enabled);
        // Terminal no necesita flag, pero mantener coherencia si más adelante se requiere
}
    
void InputFacade::setActiveInputMode(const String& mode) {
        activeMode = mode;
    }

String InputFacade::getActiveInputMode() const {
        return activeMode;
    }

void InputFacade::detectAndSwitchMode() {
    // Opcional: implementar heurística de timeout para volver a TERMINAL+JOY
        if (millis() - lastActivityTime > 30000) {
            activeMode = "TERMINAL+JOY";
        }
    }
