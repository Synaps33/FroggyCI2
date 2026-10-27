#pragma once

#include "game_level.h"
#include <memory>

namespace oci {
namespace objects {
    class Active;
}

namespace levels {

class Level_1_10 : public GameLevel {
public:
    void Init(int wave);
protected:
    virtual void Run() override;
private:
    std::weak_ptr<objects::Active> mBoss;
    int mWave;
};

} // namespace levels
} // namespace oci
