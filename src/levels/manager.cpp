/*
 * manager.cpp
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

#include "manager.h"

#include "factory.h"
#include "interface/interface.h"
#include "wave_loader.h"
#include "wave_settings.h"

#include <game/savegame.h>
#include <audio/player.h>
#include <core/boot_log.h>
#include <core/vector2.h>
#include <core/window.h>
#include <context/context.h>
#include <context/manager.h>
#include <context/object_storage.h>
#include <core/critical_error.h>
#include <game/state.h>
#include <objects/characters/player_ship.h>

#include <iostream>
#include <string>
#include <vector>

namespace oci {
namespace levels {

static inline std::string int_to_str(int v) {
    char b[16];
    snprintf(b, sizeof(b), "%d", v);
    return std::string(b);
}


Manager& Manager::Instance() {
    static Manager manager;
    return manager;
}

Manager::Manager() : current_level_index(0) {
}

namespace {


    struct LevelEntry {
        std::string name;
        int round;
        int wave;
        bool is_wave;
    };

    std::vector<LevelEntry> s_level_sequence;

    void BuildLevelSequence() {
        if(!s_level_sequence.empty())
            return;

        s_level_sequence.push_back({"intro_0", 1, 0, false});
        s_level_sequence.push_back({"start_round_1", 1, 0, false});

        for(int round = 1; round <= 10; ++round) {
            std::string prefix = "level_" + int_to_str(round) + "_";
            for(int wave = 1; wave <= 10; ++wave) {
                s_level_sequence.push_back({prefix + int_to_str(wave), round, wave, true});
            }
            if(round < 10) {
                s_level_sequence.push_back({"intro_1", round, 0, false});
                s_level_sequence.push_back({"start_round_" + int_to_str(round + 1), round + 1, 0, false});
            }
        }
        s_level_sequence.push_back({"game_over", 10, 0, false});
    }
}

void Manager::NewGame() {
    BuildLevelSequence();
    audio::StopAll();
    SaveGame::Invalidate();
    mGameOver = false;
    context::Manager::Instance().KillContext("game");
    context::Manager::Instance().KillContext("intro");
    context::ObjectStorage& game_global_storage =
        context::Manager::Instance().GetContext("game").GetStorage("global");
    mPlayerShip = game_global_storage.CreateObject<objects::PlayerShip>();
    game_global_storage.CreateObject<Interface>();
    State::Instance().Reset();
    SelectLevel(0);
}

void Manager::SelectLevel(int level_index) {
    BuildLevelSequence();
    std::cout << "SelectLevel " << level_index << std::endl;
    if(level_index >= static_cast<int>(s_level_sequence.size())) {
        GameOver();
        return;
    }

    const LevelEntry& entry = s_level_sequence[level_index];
    if(entry.is_wave) {
        CurrentWave() = WaveLoader::Instance().GetPlanet(entry.round);
        CurrentWave().wave = entry.wave;
    }
    Factory::Instance().Build(entry.name.c_str());
    current_level_index = level_index;
    if(level_index >= 1 && level_index < static_cast<int>(s_level_sequence.size()) - 1)
        SaveProgress();
}

void Manager::CreateNextLevel() {
    SelectLevel(current_level_index + 1);
}

void Manager::GameOver() {
    audio::StopLoops();
    SaveGame::Invalidate();
    mGameOver = true;
    Factory::Instance().Build("game_over");
}

objects::PlayerShip& Manager::GetPlayerShip() const {
    if(!mPlayerShip) 
        CriticalError("Player ship is not created");
    return *mPlayerShip;
}

bool Manager::CanSave() const {
    return !mGameOver && mPlayerShip &&
           current_level_index >= 1 &&
           current_level_index < static_cast<int>(s_level_sequence.size()) - 1;
}

void Manager::SaveProgress() {
    if(!CanSave())
        return;

    SaveData data;
    data.valid = true;
    data.score = State::Instance().Score();
    data.lives = State::Instance().Lives();
    data.missiles = State::Instance().Missiles();
    mPlayerShip->ExportGun(data.gun_type, data.gun_level);
    data.level = current_level_index;
    SaveGame::Write(data);
}

void Manager::RequestSave() {
    if(CanSave())
        SaveGame::MarkDirty();
}

void Manager::TickAutosave() {
    if(!SaveGame::IsDirty())
        return;
    static int counter = 0;
    if(++counter >= 50) {
        counter = 0;
        SaveProgress();
    }
}

void Manager::FlushProgress() {
    if(SaveGame::IsDirty())
        SaveProgress();
}

void Manager::ContinueGame() {
    BuildLevelSequence();
    SaveData data;
    if(!SaveGame::Load(data) || !data.valid) {
        BootLog("continue: no valid save");
        NewGame();
        return;
    }
    BootLog("continue: level=%d score=%d lives=%d missiles=%d gun=%d/%d",
            data.level, data.score, data.lives, data.missiles,
            data.gun_type, data.gun_level);

    context::Manager::Instance().KillContext("game");
    context::Manager::Instance().KillContext("intro");

    audio::StopAll();
    mGameOver = false;
    State::Instance().Restore(data.score, data.lives, data.missiles);

    context::ObjectStorage& game_global_storage =
        context::Manager::Instance().GetContext("game").GetStorage("global");
    mPlayerShip = game_global_storage.CreateObject<objects::PlayerShip>();
    game_global_storage.CreateObject<Interface>();
    mPlayerShip->ImportGun(data.gun_type, data.gun_level);
    mPlayerShip->setPosition(Vector2f(Window::Instance().getSize().x / 2.0f,
                                      objects::PlayerShip::DEFAULT_Y_POS()));

    int idx = data.level;
    if(idx < 1 || idx >= static_cast<int>(s_level_sequence.size()) - 1)
        idx = 1;
    SelectLevel(idx);
}

} // namespace levels
} // namespace oci
