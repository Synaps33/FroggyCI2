/*
 * level_1_10.cpp
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

#include "level_1_10.h"

#include "factory.h"
#include "game_level.h"
#include "level.h"
#include "wave_settings.h"
#include <context/object_storage.h>
#include <core/vector2.h>
#include <core/window.h>
#include <objects/characters/big_chicken.h>
#include <objects/characters/boss1.h>
#include <objects/characters/mother_hen.h>

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

namespace {


    static const float BOSS_INITIAL_Y_POS = -60.0f;
    static const float BOSS_INITIAL_DESTINATION_POINT_Y = 60.0f;

    static Factory::Registrar<Level_1_10> reg1("level_1_10", "game", 10);
    static Factory::Registrar<Level_1_10> reg2("level_2_10", "game", 20);
    static Factory::Registrar<Level_1_10> reg3("level_3_10", "game", 30);
    static Factory::Registrar<Level_1_10> reg4("level_4_10", "game", 40);
    static Factory::Registrar<Level_1_10> reg5("level_5_10", "game", 50);
    static Factory::Registrar<Level_1_10> reg6("level_6_10", "game", 60);
    static Factory::Registrar<Level_1_10> reg7("level_7_10", "game", 70);
    static Factory::Registrar<Level_1_10> reg8("level_8_10", "game", 80);
    static Factory::Registrar<Level_1_10> reg9("level_9_10", "game", 90);
    static Factory::Registrar<Level_1_10> reg10("level_10_10", "game", 100);
} // namespace

void Level_1_10::Init(int /*wave*/) {
    GameLevel::Init("level_1_10");
    const WaveSettings& settings = CurrentWave();
    mWave = (settings.round - 1) * 10 + settings.wave;
    const Vector2u wndSize = Window::Instance().getSize();
    const Vector2f start(wndSize.x / 2, BOSS_INITIAL_Y_POS);
    const Vector2f destination(wndSize.x / 2, BOSS_INITIAL_DESTINATION_POINT_Y);
    float speed = settings.boss_speed + 0.1f * settings.wave;
    int health = 50 + (settings.round - 1) * 20 + settings.wave * 2;

    const bool big = (mWave == 20 || mWave == 80 || mWave == 100);
    const bool mother = (mWave == 40 || mWave == 70);
    if(big) {
        health += 40;
        mBoss = Storage().CreateObject<BigChicken>(start, destination, speed, health);
    } else if(mother) {
        health += 80;
        speed *= 0.8f;
        mBoss = Storage().CreateObject<MotherHen>(start, destination, speed, health);
    } else {
        mBoss = Storage().CreateObject<Boss1>(start, destination, speed, health);
    }
    std::string w_str = int_to_str(mWave);
    ShowLevelInfo(w_str.c_str(), WaveName(settings.round, settings.wave, "show'em who's boss!"));
}

void Level_1_10::Run() {
    Level::Run();
    if(mBoss.expired())
        EndLevel();
}

} // namespace levels
} // namespace oci
