//
// Created by Kit on 8/14/2024.
//

#ifndef DIMMER_H
#define DIMMER_H

#include <Arduino.h>

typedef void (*ChangeHandler)(const float new_value);

class Dimmer {
    pin_size_t p;
    int precision;
    float max_value;

    float previous_reading;
    float trigger_delta;

    ChangeHandler handler;
public:
    Dimmer(pin_size_t pin, int precision, int max_reading, float trigger_delta);
    explicit Dimmer(pin_size_t pin); // defaults to precision=10

    void begin(); // initializes the pin to read mode, sets the ADC precision
    float read() const; // queries the sensor, and returns a value in the range [0..1]

    float poll(); // returns -1 if the poll interval has not expired yet. Otherwise returns the value of the sensor.

    void register_change_handler(ChangeHandler h);
};



#endif //DIMMER_H
