/*
 * level_1_1.cpp
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

#include "level_1_1.h"

#include "factory.h"
#include "game_level.h"
#include "level.h"
#include "wave_settings.h"
#include <constants.h>
#include <context/object_storage.h>
#include <core/vector2.h>
#include <core/window.h>
#include <font/font.h>
#include <objects/characters/chicken.h>
#include <objects/characters/random_chicken.h>
#include <objects/modifiers/expiring.h>
#include <objects/text/text.h>

#include <cstdlib>
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


    const float INITIAL_CHICKEN_Y_POS = -30.0f;

    Factory::Registrar<Level_1_1> reg1("level_1_1", "game", 1);
    Factory::Registrar<Level_1_1> reg2("level_2_1", "game", 11);
    Factory::Registrar<Level_1_1> reg3("level_3_1", "game", 21);
    Factory::Registrar<Level_1_1> reg4("level_4_1", "game", 31);
    Factory::Registrar<Level_1_1> reg5("level_5_1", "game", 41);
    Factory::Registrar<Level_1_1> reg6("level_6_1", "game", 51);
    Factory::Registrar<Level_1_1> reg7("level_7_1", "game", 61);
    Factory::Registrar<Level_1_1> reg8("level_8_1", "game", 71);
    Factory::Registrar<Level_1_1> reg9("level_9_1", "game", 81);
    Factory::Registrar<Level_1_1> reg10("level_10_1", "game", 91);
} // namespace

void Level_1_1::Init(int /*wave*/) {
    GameLevel::Init("level_1_1");
    const WaveSettings& settings = CurrentWave();
    mMode = 0;
    mWave = (settings.round - 1) * 10 + settings.wave;
    mRound = settings.round;
    mWaveNo = settings.wave;
    const int health = settings.chicken_health + (settings.wave - 1) / 4;
    const float speed = settings.chicken_speed + 0.05f * (settings.wave - 1);
    const int count = 1 + (settings.round - 1) / 3;
    for(int i = 0; i < count; ++i) {
        mChickens.push_back(Storage().CreateObject<RandomChicken>(
            Vector2f(static_cast<float>(std::rand() % Window::Instance().getSize().x),
                     INITIAL_CHICKEN_Y_POS - i * 45.0f), settings.chicken_period,
            settings.chicken_type, health, speed));
    }
}

void Level_1_1::Run() {
    Level::Run();
    if(mMode < 50) {
        ++mMode;
        if(mMode < 40) {
            if(mMode == 10 && CurrentWave().round == 1 && CurrentWave().wave == 1) {
                const int window_center = Window::Instance().getSize().x / 2;
                Storage().CreateObject<modifiers::Expiring<Text> >(
                    20, "on approach to",
                    Vector2f(window_center, FirstTextStringY),
                    Font::GetFont("medium.xml"),
                    Text::haCenter, Text::vaCenter);
                Storage().CreateObject<modifiers::Expiring<Text> >(
                    20, "pluto",
                    Vector2f(window_center, SecondTextStringY),
                    Font::GetFont("big.xml"),
                    Text::haCenter, Text::vaCenter);
            }
        } else if(mMode == 50) {
            std::string w_str = int_to_str(mWave);
            ShowLevelInfo(w_str.c_str(), WaveName(mRound, mWaveNo, "first contact"));
        }
    } else {
        bool alive = false;
        for(const std::weak_ptr<Chicken>& chicken : mChickens)
            alive = alive || !chicken.expired();
        if(!alive)
            EndLevel();
    }
}

} // namespace levels
} // namespace oci
