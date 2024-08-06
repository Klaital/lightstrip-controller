//
// Created by Kit on 8/5/2024.
//

#ifndef LIGHTSTRIP_H
#define LIGHTSTRIP_H

#include <Arduino.h>

constexpr int WarmWhite[4] = {255, 200, 255, 100};
constexpr int LightsOff[4] = {0, 0, 0, 0};

class Lightstrip {
private:
    pin_size_t pins[4] = {2,3,4,5};
    int power[4] = {0,0,0,0};
    float dimmer = 1.0;
public:
    explicit Lightstrip(const pin_size_t pins[4]);

    void begin();
    void set_power(const int new_settings[4], float dimmer);
    void drive() const; // update the pins
};

#endif //LIGHTSTRIP_H
