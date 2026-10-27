#pragma once

#include "game_level.h"
#include <core/vector2.h>
#include <objects/characters/chicken.h>

#include <list>
#include <memory>

namespace oci {
namespace levels {

class Level_1_2 : public GameLevel {
public:

    class LevelChicken : public objects::Chicken {
    public:
        void Init(float x, float y, int xpos, int ypos, char Mode, float y0,
                  Chicken::Type type, int health, int period) {
            Chicken::Init(Vector2f(x, y0), period, type, health);
            X = x;
            Y = y;
            xPos = xpos;
            yPos = ypos;
            mode = Mode;
        }

        float X, Y;
        int xPos, yPos;
        char mode;
    };

    void Init(int wave);

private:
    std::list<std::weak_ptr<LevelChicken> > mChickens;

    virtual void Run() override;

    char mode;
    float dx;
    int mWave;
};

} // namespace levels
} // namespace oci
