/*
 * level_1_5.cpp
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

#include "level_1_5.h"

#include "factory.h"
#include "game_level.h"
#include "level.h"
#include "wave_settings.h"
#include <context/object_storage.h>
#include <core/vector2.h>
#include <core/window.h>
#include <objects/characters/paratrooper.h>
#include <utils/cleanup_container.h>

#include <cstdlib>
#include <functional>
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

static Factory::Registrar<CLevel_1_5> reg1_5("level_1_5", "game", 5);
static Factory::Registrar<CLevel_1_5> reg1_6("level_1_6", "game", 6);
static Factory::Registrar<CLevel_1_5> reg1_7("level_1_7", "game", 7);
static Factory::Registrar<CLevel_1_5> reg2_5("level_2_5", "game", 15);
static Factory::Registrar<CLevel_1_5> reg2_6("level_2_6", "game", 16);
static Factory::Registrar<CLevel_1_5> reg2_7("level_2_7", "game", 17);
static Factory::Registrar<CLevel_1_5> reg3_5("level_3_5", "game", 25);
static Factory::Registrar<CLevel_1_5> reg3_6("level_3_6", "game", 26);
static Factory::Registrar<CLevel_1_5> reg3_7("level_3_7", "game", 27);
static Factory::Registrar<CLevel_1_5> reg4_5("level_4_5", "game", 35);
static Factory::Registrar<CLevel_1_5> reg4_6("level_4_6", "game", 36);
static Factory::Registrar<CLevel_1_5> reg4_7("level_4_7", "game", 37);
static Factory::Registrar<CLevel_1_5> reg5_5("level_5_5", "game", 45);
static Factory::Registrar<CLevel_1_5> reg5_6("level_5_6", "game", 46);
static Factory::Registrar<CLevel_1_5> reg5_7("level_5_7", "game", 47);
static Factory::Registrar<CLevel_1_5> reg6_5("level_6_5", "game", 55);
static Factory::Registrar<CLevel_1_5> reg6_6("level_6_6", "game", 56);
static Factory::Registrar<CLevel_1_5> reg6_7("level_6_7", "game", 57);
static Factory::Registrar<CLevel_1_5> reg7_5("level_7_5", "game", 65);
static Factory::Registrar<CLevel_1_5> reg7_6("level_7_6", "game", 66);
static Factory::Registrar<CLevel_1_5> reg7_7("level_7_7", "game", 67);
static Factory::Registrar<CLevel_1_5> reg8_5("level_8_5", "game", 75);
static Factory::Registrar<CLevel_1_5> reg8_6("level_8_6", "game", 76);
static Factory::Registrar<CLevel_1_5> reg8_7("level_8_7", "game", 77);
static Factory::Registrar<CLevel_1_5> reg9_5("level_9_5", "game", 85);
static Factory::Registrar<CLevel_1_5> reg9_6("level_9_6", "game", 86);
static Factory::Registrar<CLevel_1_5> reg9_7("level_9_7", "game", 87);
static Factory::Registrar<CLevel_1_5> reg10_5("level_10_5", "game", 95);
static Factory::Registrar<CLevel_1_5> reg10_6("level_10_6", "game", 96);
static Factory::Registrar<CLevel_1_5> reg10_7("level_10_7", "game", 97);

inline void CLevel_1_5::CreateParatrooper(float x, float y) {
    const WaveSettings& settings = CurrentWave();
    const float speed = settings.paratrooper_speed + 0.05f * settings.wave;
    const int health = settings.paratrooper_health + (settings.wave - 1) / 4;
    std::weak_ptr<Paratrooper> paratrooper = Storage().CreateObject<Paratrooper>(
        Vector2f(x, y), speed, settings.paratrooper_period, health,
        settings.chicken_type);
    std::shared_ptr<Paratrooper>(paratrooper)->SetOnCreateRandomChickenCallback(
        std::bind(static_cast<
                void(ChickensList::*)(const ChickensList::value_type&)
            >(&ChickensList::push_front), mChickens, std::placeholders::_1));
    mChickens.push_front(paratrooper);
}

void CLevel_1_5::Init(int /*wave*/) {
    GameLevel::Init("level_1_5");
    const WaveSettings& settings = CurrentWave();
    mWave = (settings.round - 1) * 10 + settings.wave;
    const Vector2u wnd_size = Window::Instance().getSize();
    int sub = settings.wave;
    std::string w_str = int_to_str(mWave);
    switch(sub) {
    case 5:
        ShowLevelInfo(w_str.c_str(), WaveName(settings.round, settings.wave, "paratroopers!"));
        for(int y = 0; y < 4; ++y) {
            float y0 = -40.0f - 150.0f * y;
            float dy = 40;
            for(int x = 0; x < 7; ++x) {
                CreateParatrooper(x * wnd_size.x / 7 + 32.0f, y0);
                y0 += dy;
                dy -= 14;
            }
        }
        break;
    case 6:
        ShowLevelInfo(w_str.c_str(), WaveName(settings.round, settings.wave, "death from above"));
        for(int n = 0; n < 16; ++n) {
            int x = std::rand() % wnd_size.x;
            int y = -(std::rand() % wnd_size.y);
            CreateParatrooper(x, y);
        } break;
    case 7:
    default:
        {
            ShowLevelInfo(w_str.c_str(), WaveName(settings.round, settings.wave, "third time lucky"));
            float y0 = -20;
            for(int n = 0; n < 3; ++n) {
                float x0 = wnd_size.x - 30;
                for(int k = 0; k < 6; ++k) {
                    CreateParatrooper(x0, y0);
                    x0 -= 100 + rand() % 20;
                    y0 -= 40;
                }
                y0 -= 240;
            }
            y0 = -260;
            for(int n = 0; n < 3; ++n) {
                float x0 = 30;
                for(int k = 0; k < 6; ++k) {
                    CreateParatrooper(x0, y0);
                    x0 += 100 + rand() % 20;
                    y0 -= 40;
                }
                y0 -= 240;
            }
        } break;
    }
    const int extra = (settings.round - 1) * 2;
    for(int n = 0; n < extra; ++n)
        CreateParatrooper(static_cast<float>(std::rand() % wnd_size.x),
                          -static_cast<float>(std::rand() % wnd_size.y));
}

void CLevel_1_5::Run() {
    Level::Run();
    cleanup(mChickens);
    if(mChickens.empty())
        EndLevel();
}

} // namespace levels
} // namespace oci
