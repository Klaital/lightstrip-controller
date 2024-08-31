//
// Created by Kit on 8/5/2024.
//

#include "Lightstrip.h"

Lightstrip::Lightstrip(const pin_size_t pins[4]) {
    for (int i=0; i<4; i++) {
        this->pins[i] = pins[i];
    }
}

void Lightstrip::begin() {
    for (auto p: this->pins) {
        pinMode(p, OUTPUT);
        analogWrite(p, 0);
    }
}

void Lightstrip::set_power(const int new_settings[4], const float dimmer) {
    this->dimmer = dimmer;
    if (dimmer < 0.0f) {
        this->dimmer = 0.0f;
    }
    if (dimmer > 1.0f) {
        this->dimmer = 1.0f;
    }
    for (int i=0; i<4; i++) {
        this->power[i] = new_settings[i];
    }
}

void Lightstrip::toggle_power() {
    if (this->dimmer > 0.0f) {
        this->dimmer = 0.0f;
    } else {
        this->dimmer = 1.0f;
    }
}


void Lightstrip::drive() const {
    for (int i=0; i<4; i++) {
        const int pwr = static_cast<int>(
            static_cast<float>(this->power[i]) * this->dimmer
        );
        analogWrite(this->pins[i], pwr);
    }
}
