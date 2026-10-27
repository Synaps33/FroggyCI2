/*
 * level_1_4.cpp
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

#include "level_1_4.h"

#include "factory.h"
#include "level_x4.h"
#include "wave_settings.h"
#include <context/object_storage.h>
#include <core/vector2.h>
#include <core/window.h>
#include <objects/characters/asteroid.h>
#include <objects/modifiers/auto_killable.h>

#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>

namespace oci {
namespace levels {

static inline std::string int_to_str(int v) {
    char b[16];
    snprintf(b, sizeof(b), "%d", v);
    return std::string(b);
}




using namespace objects;
using namespace modifiers;

static Factory::Registrar<Level_1_4> reg1("level_1_4", "game", 4);
static Factory::Registrar<Level_1_4> reg2("level_2_4", "game", 14);
static Factory::Registrar<Level_1_4> reg3("level_3_4", "game", 24);
static Factory::Registrar<Level_1_4> reg4("level_4_4", "game", 34);
static Factory::Registrar<Level_1_4> reg5("level_5_4", "game", 44);
static Factory::Registrar<Level_1_4> reg6("level_6_4", "game", 54);
static Factory::Registrar<Level_1_4> reg7("level_7_4", "game", 64);
static Factory::Registrar<Level_1_4> reg8("level_8_4", "game", 74);
static Factory::Registrar<Level_1_4> reg9("level_9_4", "game", 84);
static Factory::Registrar<Level_1_4> reg10("level_10_4", "game", 94);

void Level_1_4::Init(int /*wave*/) {
    const WaveSettings& settings = CurrentWave();
    mWave = (settings.round - 1) * 10 + settings.wave;
    Level_x4::Init(10, settings.asteroid_count + settings.wave * 2);
    std::string wave_str = int_to_str(mWave);
    ShowLevelInfo(wave_str.c_str(), WaveName(settings.round, settings.wave, "meteor shower"), "watch out!", true);
}

std::weak_ptr<objects::Asteroid> Level_1_4::CreateAsteroid() {
    const Vector2u wndSize = Window::Instance().getSize();
    const WaveSettings& settings = CurrentWave();
    int x = wndSize.x + 10;
    int y = -10;
    if(std::rand() % 3)
        x = std::rand() % wndSize.x * 3 / 4 + wndSize.x / 4;
    else
        y = std::rand() % (wndSize.y / 2);
    bool big = std::rand() & 1;
    const float speed = settings.asteroid_speed + 0.12f * settings.wave;
    const int big_health = 5 + settings.round + settings.wave / 3;
    const int small_health = 2 + settings.round / 2;
    return Storage().CreateObject<AutoKillable<Asteroid, BoundLeft, BoundBottom> >(
                Vector2f(x, y),
                6.1f,
                speed,
                big ? big_health : small_health,
                big ? objects::Asteroid::tRockBig :
                      objects::Asteroid::tRockSmall);
}

} // namespace levels
} // namespace oci
