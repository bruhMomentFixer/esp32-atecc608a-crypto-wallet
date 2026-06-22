#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>

class InputManager {
public:
    enum InputEvent {
        NO_EVENT,
        NAV_UP,
        NAV_DOWN,
        NAV_LEFT,
        NAV_RIGHT,
        NAV_SELECT,
        PIN_DIGIT,
        PIN_BACKSPACE,
        PIN_CONFIRM
    };
    
    struct InputData {
        InputEvent event;
        char digit;
        int menuItem;
    };

    InputManager(int joyYPin, int joyBtnPin);
    void initialize();
    InputData processInputs();
    void setPinInputMode(bool enabled);
    
public:
    int joyYPin, joyBtnPin;
    bool pinInputMode;
    int lastYValue;
    int lastXValue;
    bool lastButtonState;
    unsigned long lastActionTime;
    
    static const unsigned long ACTION_COOLDOWN = 300;
    static const unsigned long PIN_VISIBLE_TIME = 1000;
    
    InputData processJoystick();
    InputData processTerminal();
    bool detectJoystickActivity();

    // ✅ NUEVAS FUNCIONES — añade justo aquí
    void calibrate();                      // Lee joystick y fija valores iniciales estables
    void suppressInitialInputs(unsigned long millis_from_now); // Ignora entradas por X ms
};

#endif
