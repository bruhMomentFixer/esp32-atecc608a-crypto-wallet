#ifndef INPUT_FACADE_H
#define INPUT_FACADE_H

#include "InputManager.h"

class InputFacade {
public:
    struct InputData {
        InputManager::InputEvent event;
        char digit;
        int menuItem;
    };

    InputFacade(InputManager* jm);
    void initialize();
    InputData processInputs(); // Unifica terminal + joystick
    void setPinInputMode(bool enabled);
    void setActiveInputMode(const String& mode);
    String getActiveInputMode() const;

private:
    InputManager* joystick;
    String activeMode; // e.g. "TERMINAL", "JOYSTICK", "TERMINAL+JOY"
    unsigned long lastActivityTime;
    void detectAndSwitchMode();
};

#endif
