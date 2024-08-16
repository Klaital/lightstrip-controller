//
// Created by Kit on 8/14/2024.
//

#include "Dimmer.h"

#include <WiFi.h>

Dimmer::Dimmer(const pin_size_t pin):
    p(pin),
    precision(10),
    max_value(1024.0f),
    previous_reading(0.0f),
    trigger_delta(0.05f),
    handler(nullptr)
{
}

Dimmer::Dimmer(const pin_size_t pin, const int precision, const int max_reading, const float trigger_delta):
    p(pin),
    precision(precision),
    previous_reading(0.0f),
    trigger_delta(trigger_delta),
    handler(nullptr)
{
    this->max_value = static_cast<float>(max_reading);
}

void Dimmer::begin() {
    pinMode(this->p, INPUT_PULLDOWN);
    analogReadResolution(this->precision);
    this->previous_reading = this->read();
}

float Dimmer::read() const {
    const auto d = analogRead(this->p);
    return static_cast<float>(d) / this->max_value;
}

float Dimmer::poll() {
    auto new_reading = this->read();
    if (new_reading < 0.06) {
        new_reading = 0.0f;
    }
    if (new_reading > 0.95) {
        new_reading = 1.0f;
    }

    const float delta = new_reading - this->previous_reading;
    // take action if the reading has changed,
    // or is effectively at the min or max value.
    if (delta > 0.05
        || delta < -0.05
        || (new_reading > 0.99 && this->previous_reading < 0.99)
        || (new_reading < 0.01 && this->previous_reading > 0.01)
    ) {
        this->previous_reading = new_reading;
        if (this->handler != nullptr) {
            this->handler(new_reading);
        }
    }

    return new_reading;
}

void Dimmer::register_change_handler(ChangeHandler h) {
    this->handler = h;
}
