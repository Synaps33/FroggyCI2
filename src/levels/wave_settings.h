#pragma once

#include <objects/characters/chicken.h>

namespace oci {
namespace levels {

struct WaveSettings {
    int round = 1;
    int wave = 1;
    objects::Chicken::Type chicken_type = objects::Chicken::tRed;
    float chicken_speed = 2.0f;
    float flock_speed = 1.0f;
    int chicken_health = 1;
    int chicken_period = 100;
    int asteroid_count = 20;
    float asteroid_speed = 8.0f;
    float paratrooper_speed = 2.0f;
    int paratrooper_health = 1;
    int paratrooper_period = 400;
    float ufo_speed = 4.0f;
    float boss_speed = 4.0f;
};

WaveSettings& CurrentWave();
const char* WaveName(int round, int wave, const char* fallback);

} // namespace levels
} // namespace oci
