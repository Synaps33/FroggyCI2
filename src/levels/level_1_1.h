#pragma once

#include "game_level.h"
#include <objects/characters/chicken.h>
#include <memory>
#include <vector>

namespace oci {
namespace levels {

class Level_1_1 : public GameLevel {
public:
    void Init(int wave);
protected:
    virtual void Run() override;
private:
    int mMode;
    int mWave;
    int mRound;
    int mWaveNo;
    std::vector<std::weak_ptr<objects::Chicken> > mChickens;
};

} // namespace levels
} // namespace oci
