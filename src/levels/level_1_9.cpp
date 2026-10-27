/*
 * level_1_9.cpp
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

#include "game_level.h"
#include "level.h"
#include "wave_settings.h"
#include <context/object_storage.h>
#include <core/critical_error.h>
#include <levels/factory.h>
#include <objects/characters/ufo.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace oci {
namespace levels {

static inline std::string int_to_str(int v) {
    char b[16];
    snprintf(b, sizeof(b), "%d", v);
    return std::string(b);
}




class Level_1_9 : public GameLevel {
public:
    void Init(int /*wave*/) {
        GameLevel::Init("level_1_9");
        const WaveSettings& settings = CurrentWave();
        const int wave_number = (settings.round - 1) * 10 + settings.wave;
        const float ufo_speed = settings.ufo_speed + 0.15f * settings.wave;
        const int count = 1 + (settings.round - 1) / 4;
        for(int i = 0; i < count; ++i)
            mUFOs.push_back(Storage().CreateObject<objects::UFO>(ufo_speed));
        std::string w_str = int_to_str(wave_number);
        ShowLevelInfo(w_str.c_str(), WaveName(settings.round, settings.wave, "bonus"));
    }

protected:
    void Run() override {
        Level::Run();
        bool alive = false;
        for(const std::weak_ptr<objects::UFO>& ufo : mUFOs)
            alive = alive || !ufo.expired();
        if(!alive)
            EndLevel();
    }
private:
    std::vector<std::weak_ptr<objects::UFO> > mUFOs;
};

Factory::Registrar<Level_1_9> reg1("level_1_9", "game", 9);
Factory::Registrar<Level_1_9> reg2("level_2_9", "game", 19);
Factory::Registrar<Level_1_9> reg3("level_3_9", "game", 29);
Factory::Registrar<Level_1_9> reg4("level_4_9", "game", 39);
Factory::Registrar<Level_1_9> reg5("level_5_9", "game", 49);
Factory::Registrar<Level_1_9> reg6("level_6_9", "game", 59);
Factory::Registrar<Level_1_9> reg7("level_7_9", "game", 69);
Factory::Registrar<Level_1_9> reg8("level_8_9", "game", 79);
Factory::Registrar<Level_1_9> reg9("level_9_9", "game", 89);
Factory::Registrar<Level_1_9> reg10("level_10_9", "game", 99);

} // namespace levels
} // namespace oci
