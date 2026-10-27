/*
 * mainmenu.cpp
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

#include "mainmenu.h"

#include "background_controller.h"
#include "cursor.h"
#include <background/background.h>
#include <context/context.h>
#include <context/manager.h>
#include <context/object_holder.h>
#include <context/object_storage.h>
#include <core/boot_log.h>
#include <core/keyboard.h>
#include <core/mouse.h>
#include <core/vector2.h>
#include <core/window.h>
#include <core/video_settings.h>
#include <font/font.h>
#include <game/savegame.h>
#include <audio/player.h>
#include <levels/manager.h>
#include <resources/loader.h>
#include <objects/base/active.h>
#include <objects/base/sprite.h>
#include <objects/base/visible.h>
#include <objects/effects/unshade_screen.h>
#include <objects/text/text.h>

// IWYU pragma: no_include <__fwd/string_view.h>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace oci {

using namespace objects;

namespace {

    const char MENU_CONTEXT_NAME[] = "menu";
    static int s_selected_btn = 0;
    static int s_button_count = 2;
    static bool s_prev_up = false;
    static bool s_prev_down = false;
    static bool s_prev_enter = false;
    static bool s_prev_escape = false;
    static bool s_in_options = false;
    enum class PendingPage { None, Main, Options };
    static PendingPage s_pending_page = PendingPage::None;
    // Requires a fresh press after the menu is (re)created, so a held button
    // from gameplay does not activate an entry on release.
    static bool s_input_armed = false;

    void RebuildMenuPage(bool options) {
        s_in_options = options;
        s_selected_btn = 0;
        context::Manager& manager = context::Manager::Instance();
        manager.KillContext(MENU_CONTEXT_NAME);
        MainMenu::InitMenu();
    }

    class Button : public Sprite, public Active {
    public:

        enum State {
            sNormal,
            sChecked,
            sHover,
            sCheckedHover
        };

        Button() : mIndex(0) {}

        void Init(int index, std::string_view caption, int ypos,
                  const std::function<void()>& on_press_callback) {
            mIndex = index;
            Sprite::Init("menuitem.xml");
            setPosition(Window::Instance().getSize().x / 2, ypos);
            mCaption = Storage().CreateObject<Text>(
                caption, getPosition(), Font::GetFont("medium.xml"),
                Text::haCenter, Text::vaCenter);
            mOnPressCallback = on_press_callback;
        }

        DrawPriority GetDrawPriority() const override {
            return dpControlBackground;
        }

        void Run() override {
            const bool is_selected = (s_selected_btn == mIndex);
            SetState(is_selected ? sHover : sNormal);

            if(is_selected && s_input_armed && s_prev_enter &&
               !Keyboard::isKeyPressed(Keyboard::Return) &&
               !Keyboard::isKeyPressed(Keyboard::Fire)) {
                mOnPressCallback();
            }
        }

    private:
        int mIndex;
        ObjectHolder<Text> mCaption;
        std::function<void()> mOnPressCallback;
    };

    class MenuController : public Active {
    public:
        void Run() override {
            bool up = Keyboard::isKeyPressed(Keyboard::Up);
            bool down = Keyboard::isKeyPressed(Keyboard::Down);
            bool enter = Keyboard::isKeyPressed(Keyboard::Return) || Keyboard::isKeyPressed(Keyboard::Fire);
            bool escape = Keyboard::isKeyPressed(Keyboard::Escape);

            if(s_button_count > 0) {
                if(down && !s_prev_down)
                    s_selected_btn = (s_selected_btn + 1) % s_button_count;
                if(up && !s_prev_up)
                    s_selected_btn = (s_selected_btn + s_button_count - 1) % s_button_count;
            }

            if(s_in_options && escape && !s_prev_escape)
                MainMenu::CloseOptionsMenu();

            if(!enter)
                s_input_armed = true;

            s_prev_up = up;
            s_prev_down = down;
            s_prev_enter = enter;
            s_prev_escape = escape;
        }
    };

    class Logo : public CommonSprite<Visible::dpForeground> {
    public:
        void Init() {
            Sprite::Init("logo.xml");
            setPosition(Vector2f(Window::Instance().getSize().x / 2, 96));
        }
    };

    inline void doexit() {
        Window::Instance().close();
        std::exit(EXIT_SUCCESS);
    }
} // namespace

void MainMenu::Init() {
    s_prev_up = s_prev_down = s_prev_enter = s_prev_escape = false;
    s_input_armed = false;
    BootLog("menu: background");
    Background::Instance().SetController(
        Storage().CreateObject<menu::BackgroundController>());

    BootLog("menu: logo/buttons");
    Storage().CreateObject<MenuController>();
    Storage().CreateObject<Logo>();

    int index = 0;
    int y_start = s_in_options ? 262 : 258;
    const int y_step = 42;

    if(!s_in_options) {
        if(SaveGame::Exists()) {
            BootLog("menu: continue available");
            Storage().CreateObject<Button>(index++, "continue", y_start,
                std::bind(&levels::Manager::ContinueGame, &levels::Manager::Instance()));
            y_start += y_step;
        }
        Storage().CreateObject<Button>(index++, "save the world", y_start,
                std::bind(&levels::Manager::NewGame, &levels::Manager::Instance()));
        y_start += y_step;

        Storage().CreateObject<Button>(index++, "options", y_start, []() {
            MainMenu::OpenOptionsMenu();
        });
        y_start += y_step;

        Storage().CreateObject<Button>(index++, "quit", y_start, doexit);
    } else {
        std::string res_label = std::string("res: ") + VideoSettings::GetResolutionName();
        Storage().CreateObject<Button>(index++, res_label, y_start, []() {
            VideoSettings::ToggleResolution();
            MainMenu::OpenOptionsMenu();
        });
        y_start += y_step;

        Storage().CreateObject<Button>(index++, "back", y_start, []() {
            MainMenu::CloseOptionsMenu();
        });
    }

    s_button_count = index;
    if(s_selected_btn >= s_button_count)
        s_selected_btn = 0;

    BootLog("menu: labels/fonts");
    Storage().CreateObject<Text>(
        s_in_options ? "options" : "main menu",
        Vector2f(Window::Instance().getSize().x / 2, s_in_options ? 198 : 208),
        Font::GetFont("big.xml"), Text::haCenter, Text::vaCenter);

    if(s_in_options) {
        resources::ResourceLoader& loader = resources::ResourceLoader::Instance();
        std::string dat = "dat: " + loader.PrimaryArchiveName();
        dat += loader.IsFullVersion() ? " (full)" : " (demo)";
        Storage().CreateObject<Text>(
            dat, Vector2f(Window::Instance().getSize().x / 2, 232),
            Font::GetFont("small.xml"), Text::haCenter, Text::vaCenter);
    }

    Storage().CreateObject<UnshadeScreen>();
    BootLog("menu: done");
}

void MainMenu::Run() {
}

void MainMenu::SwitchToMenu() {
    s_in_options = false;
    s_selected_btn = 0;
    context::Manager& manager = context::Manager::Instance();
    audio::StopAll();
    levels::Manager::Instance().FlushProgress();
    manager.KillContext(MENU_CONTEXT_NAME);
    InitMenu();
}

void MainMenu::OpenOptionsMenu() {
    s_pending_page = PendingPage::Options;
}

void MainMenu::CloseOptionsMenu() {
    s_pending_page = PendingPage::Main;
}

void MainMenu::ProcessPending() {
    if(s_pending_page == PendingPage::None)
        return;
    PendingPage page = s_pending_page;
    s_pending_page = PendingPage::None;
    RebuildMenuPage(page == PendingPage::Options);
}

void MainMenu::InitMenu() {
    context::Context& context =
        context::Manager::Instance().GetContext(MENU_CONTEXT_NAME);
    context.GetStorage(context::storage::LOCAL).CreateObject<MainMenu>();
    context::Manager::Instance().SetActiveContext(context);
}

} // namespace oci
