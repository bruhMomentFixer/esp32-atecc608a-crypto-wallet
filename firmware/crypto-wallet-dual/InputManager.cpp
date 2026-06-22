#include "InputManager.h"

InputManager::InputManager(int joyYPin, int joyBtnPin) : 
    joyYPin(joyYPin), joyBtnPin(joyBtnPin), pinInputMode(false),
    lastYValue(2000), lastButtonState(HIGH), lastActionTime(0) {}

void InputManager::initialize() {
    pinMode(joyBtnPin, INPUT_PULLUP);
    pinMode(joyYPin, INPUT);
}

void InputManager::calibrate() {
    // Tomar varias lecturas y promediar para evitar picos
    const int samples = 8;
    long sum = 0;
    for (int i = 0; i < samples; ++i) {
        sum += analogRead(joyYPin);
        delay(5);
    }
    lastYValue = (int)(sum / samples);

    // Leer estado actual del botón
    lastButtonState = digitalRead(joyBtnPin);

    // Evitar reacciones inmediatas
    lastActionTime = millis(); // bloquea por ACTION_COOLDOWN si ya implementado
}

void InputManager::suppressInitialInputs(unsigned long millis_from_now) {
    // Fija lastActionTime al futuro para ignorar inputs durante ese intervalo
    lastActionTime = millis() + millis_from_now;
}

InputManager::InputData InputManager::processInputs() {
    // Leer primero del terminal
    InputData terminalData = processTerminal();
    // Luego del joystick
    InputData joyData = processJoystick();

    // Si ambos devuelven algo, prioriza terminal
    if (terminalData.event != NO_EVENT && joyData.event != NO_EVENT) {
        return terminalData;
    }
    // Si solo uno devuelve evento
    if (terminalData.event != NO_EVENT) {
        return terminalData;
    }
    if (joyData.event != NO_EVENT) {
        return joyData;
    }

    // Si ninguno tiene evento
    InputData none = {NO_EVENT, 0, 0};
    return none;
}


InputManager::InputData InputManager::processTerminal() {
    InputData data = {NO_EVENT, 0, 0};
    
    while (Serial.available() > 0) {
        char c = Serial.read();
        
        if (c == 0x1B) { // ESC character
            if (Serial.available() >= 2) {
                char bracket = Serial.read();
                if (bracket == '[') {
                    char arrow = Serial.read();
                    if (!pinInputMode) {
                        if (arrow == 'A') {
                            data.event = NAV_UP;
                            return data;
                        } else if (arrow == 'B') {
                            data.event = NAV_DOWN;
                            return data;
                        } else if (arrow == 'C') {
                            data.event = NAV_RIGHT;
                            return data;
                        } else if (arrow == 'D') {
                            data.event = NAV_LEFT;
                            return data;
                        }
                    }
                }
            }
        } else if (c == '\r' || c == '\n') {
            data.event = pinInputMode ? PIN_CONFIRM : NAV_SELECT;
            return data;
        } else if (c == 0x7F || c == 0x08) {
            if (pinInputMode) {
                data.event = PIN_BACKSPACE;
                return data;
            }
        } else if (c >= '0' && c <= '9') {
            if (pinInputMode) {
                data.event = PIN_DIGIT;
                data.digit = c;
                return data;
            }
        }
    }
    
    return data;
}

InputManager::InputData InputManager::processJoystick() {
    InputData data = {NO_EVENT, 0, 0};

    int yValue = analogRead(joyYPin);
    bool buttonState = digitalRead(joyBtnPin);
    unsigned long currentTime = millis();

    const int center = 2048;      // Valor medio esperado (mitad de 0–4095)
    const int deadZone = 300;     // ± rango sin respuesta (ajustable)
    const int thresholdUp = center - deadZone;    // ~1748
    const int thresholdDown = center + deadZone;  // ~2348

    // Evita lecturas muy seguidas (debounce temporal)
    if (currentTime - lastActionTime < ACTION_COOLDOWN) {
        return data;
    }

    // --- Movimiento vertical con zona muerta ---
    if (yValue < thresholdUp) {
        // Joystick claramente hacia arriba
        if (lastYValue >= thresholdUp) {   // evita repetir mientras se mantiene
            data.event = NAV_UP;
            lastActionTime = currentTime;
        }
    } else if (yValue > thresholdDown) {
        // Joystick claramente hacia abajo
        if (lastYValue <= thresholdDown) {
            data.event = NAV_DOWN;
            lastActionTime = currentTime;
        }
    }

    // Actualiza el valor anterior (para evitar repeticiones mientras se mantiene)
    lastYValue = yValue;

    // --- Detección del botón con debounce ---
    if (buttonState == LOW && lastButtonState == HIGH) {
        delay(50); // pequeño debounce físico
        if (digitalRead(joyBtnPin) == LOW) {
            data.event = pinInputMode ? PIN_CONFIRM : NAV_SELECT;
            lastActionTime = currentTime;
        }
    }
    lastButtonState = buttonState;

    return data;
}

void InputManager::setPinInputMode(bool enabled) {
    pinInputMode = enabled;
}

bool InputManager::detectJoystickActivity() {
    int yValue = analogRead(joyYPin);
    bool btnState = digitalRead(joyBtnPin);
    return (abs(yValue - 2048) > 500) || (btnState == LOW);
}
