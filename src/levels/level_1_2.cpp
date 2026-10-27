/*
 * level_1_2.cpp
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

#include "level_1_2.h"

#include "factory.h"
#include "game_level.h"
#include "level.h"
#include "wave_settings.h"
#include <context/object_storage.h>
#include <core/vector2.h>
#include <core/window.h>
#include <utils/for_each.h>

#include <cmath>
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


static const float LINE_CELL_HEIGHT = 70.0f;

static const float CreateDX = 80;
static const float CreateDY = 200;
static const float CellSize = 60;
static const float CellOffsetX = 100;
static const float CellOffsetY = 40;

static Factory::Registrar<Level_1_2> reg1_2("level_1_2", "game", 2);
static Factory::Registrar<Level_1_2> reg1_3("level_1_3", "game", 3);
static Factory::Registrar<Level_1_2> reg2_2("level_2_2", "game", 12);
static Factory::Registrar<Level_1_2> reg2_3("level_2_3", "game", 13);
static Factory::Registrar<Level_1_2> reg3_2("level_3_2", "game", 22);
static Factory::Registrar<Level_1_2> reg3_3("level_3_3", "game", 23);
static Factory::Registrar<Level_1_2> reg4_2("level_4_2", "game", 32);
static Factory::Registrar<Level_1_2> reg4_3("level_4_3", "game", 33);
static Factory::Registrar<Level_1_2> reg5_2("level_5_2", "game", 42);
static Factory::Registrar<Level_1_2> reg5_3("level_5_3", "game", 43);
static Factory::Registrar<Level_1_2> reg6_2("level_6_2", "game", 52);
static Factory::Registrar<Level_1_2> reg6_3("level_6_3", "game", 53);
static Factory::Registrar<Level_1_2> reg7_2("level_7_2", "game", 62);
static Factory::Registrar<Level_1_2> reg7_3("level_7_3", "game", 63);
static Factory::Registrar<Level_1_2> reg8_2("level_8_2", "game", 72);
static Factory::Registrar<Level_1_2> reg8_3("level_8_3", "game", 73);
static Factory::Registrar<Level_1_2> reg9_2("level_9_2", "game", 82);
static Factory::Registrar<Level_1_2> reg9_3("level_9_3", "game", 83);
static Factory::Registrar<Level_1_2> reg10_2("level_10_2", "game", 92);
static Factory::Registrar<Level_1_2> reg10_3("level_10_3", "game", 93);

void Level_1_2::Init(int /*wave*/) {
    GameLevel::Init("level_1_1");
    const WaveSettings& settings = CurrentWave();
    mode = 0;
    dx = settings.flock_speed + 0.03f * (settings.wave - 1);
    if((settings.round + settings.wave) % 2 == 0)
        dx = -dx;
    mWave = (settings.round - 1) * 10 + settings.wave;
    const int health = settings.chicken_health + (settings.wave - 1) / 4;
    const int period = 800 - (settings.round - 1) * 15;
    const int variant = (settings.round + settings.wave) % 3;
    const int rows = 4 + variant;
    const int cols = 7 + variant;

    for(int y = 0; y < rows; ++y)
        for(int x = 0; x < cols; ++x) {
            mChickens.push_back(
                Storage().CreateObject<LevelChicken>(
                    y % 2 ? CellSize : Window::Instance().getSize().x - CellSize,
                    LINE_CELL_HEIGHT * (1 + y), x, y, 0, -(x * CreateDX + y * CreateDY),
                    settings.chicken_type, health, period
                )
            );
        }
    std::string w_str = int_to_str(mWave);
    ShowLevelInfo(w_str.c_str(), WaveName(settings.round, settings.wave,
                                            settings.wave == 2 ? "classic invasion" : "one more time"));
}

namespace {


    class MoveAndCalcEnemiesFunctor {
        int& count;
    public:
        MoveAndCalcEnemiesFunctor(int& count) : count(count) {}

        void operator() (Level_1_2::LevelChicken& enemy) {
            if(enemy.mode < 2) {
                ++count;
                enemy.move((enemy.X - enemy.getPosition().x) / 8,
                           (enemy.Y - enemy.getPosition().y) / 8);
                if(std::abs(enemy.X - enemy.getPosition().x) < 1 &&
                   std::abs(enemy.Y - enemy.getPosition().y) < 1) {
                    enemy.Y = enemy.yPos * CellSize + CellOffsetY;
                    enemy.X = enemy.xPos * CellSize + CellOffsetX;
                    enemy.mode++;
                }
            }
        }
    };

    class DetermineFlockBoundsFunctor {
        float& xmax, &xmin;
    public:
        DetermineFlockBoundsFunctor(float& xmax, float& xmin)
            : xmax(xmax), xmin(xmin) {}

        void operator() (Level_1_2::LevelChicken& enemy) {
            if(enemy.getPosition().x > xmax)
                xmax = enemy.getPosition().x;
            if(enemy.getPosition().x < xmin)
                xmin = enemy.getPosition().x;
        }
    };

    class MoveChickenFunctor {
        float dx;
    public:
        MoveChickenFunctor(float dx) : dx(dx) {}
        void operator() (Level_1_2::LevelChicken& chicken) {
            chicken.move(dx, 0);
        }
    };
} // namespace

void Level_1_2::Run() {
    Level::Run();
    if(mChickens.empty()) {
        EndLevel();
        return;
    }

    switch (mode) {
    case 0:
        {
            int count = 0;
            for_each(mChickens, MoveAndCalcEnemiesFunctor(count));
            if(!count)
                mode = 1;
        } break;
    case 1:
        {
            const Vector2u wnd_size = Window::Instance().getSize();
            float xmin = wnd_size.x * 2.0f;
            float xmax = -static_cast<float>(wnd_size.x);
            for_each(mChickens, DetermineFlockBoundsFunctor(xmax, xmin));
            if(xmax + dx > wnd_size.x - 32 || xmin + dx < 32)
                dx = -dx;
            for_each(mChickens, MoveChickenFunctor(dx));
        } break;
    }
}

} // namespace levels
} // namespace oci
