/*
 * level_1_8.cpp
 * This file is part of OCI (Open Chicken Invaders)
 *
 * Copyright (C) 2010-2014 - Aleksey Konovalov (konovalov.aleks@gmail.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "level_1_8.h"

#include "factory.h"
#include "game_level.h"
#include "level.h"
#include "wave_settings.h"
#include <context/object_storage.h>
#include <core/vector2.h>
#include <core/window.h>
#include <objects/characters/asteroid.h>
#include <objects/characters/jumping_asteroid.h>
#include <utils/cleanup_container.h>

#include <cstdlib>
#include <functional>
#include <string>
#include <string_view>

namespace oci {
namespace levels {

static inline std::string int_to_str(int v) {
    char b[16];
    snprintf(b, sizeof(b), "%d", v);
    return std::string(b);
}




static Factory::Registrar<CLevel_1_8> reg1("level_1_8", "game", 8);
static Factory::Registrar<CLevel_1_8> reg2("level_2_8", "game", 18);
static Factory::Registrar<CLevel_1_8> reg3("level_3_8", "game", 28);
static Factory::Registrar<CLevel_1_8> reg4("level_4_8", "game", 38);
static Factory::Registrar<CLevel_1_8> reg5("level_5_8", "game", 48);
static Factory::Registrar<CLevel_1_8> reg6("level_6_8", "game", 58);
static Factory::Registrar<CLevel_1_8> reg7("level_7_8", "game", 68);
static Factory::Registrar<CLevel_1_8> reg8("level_8_8", "game", 78);
static Factory::Registrar<CLevel_1_8> reg9("level_9_8", "game", 88);
static Factory::Registrar<CLevel_1_8> reg10("level_10_8", "game", 98);

static const float INITIAL_POS_Y = 20.0f;

void CLevel_1_8::Init(int /*wave*/) {
    GameLevel::Init("level_1_4");
    const WaveSettings& settings = CurrentWave();
    const int wave_number = (settings.round - 1) * 10 + settings.wave;
    const float speed = settings.asteroid_speed + 0.1f * settings.wave;
    const int health = 10 + settings.round * 2 + settings.wave;
    mAsteroids.push_front(
        Storage().CreateObject<objects::JumpingAsteroid>(
            Vector2f(static_cast<float>(std::rand() % Window::Instance().getSize().x),
                     INITIAL_POS_Y),
            1,
            speed,
            health,
            objects::Asteroid::tRockBig,
            true,
            std::bind(static_cast<
                    void(AsteroidsList::*)(const AsteroidsList::value_type&)
                >(&AsteroidsList::push_front), &mAsteroids, std::placeholders::_1)
        )
    );
    std::string w_str = int_to_str(wave_number);
    ShowLevelInfo(w_str.c_str(), WaveName(settings.round, settings.wave, "asteroids!"), "no, really", true);
}

void CLevel_1_8::Run() {
    Level::Run();
    cleanup(mAsteroids);
    if(mAsteroids.empty())
        EndLevel();
}

} // namespace levels
} // namespace oci
