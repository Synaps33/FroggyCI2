#pragma once

#include "level_x4.h"
#include <objects/characters/asteroid.h>
#include <memory>

namespace oci {
namespace levels {

class Level_1_4 : public Level_x4 {
public:
    void Init(int wave);
protected:
    virtual std::weak_ptr<objects::Asteroid> CreateAsteroid() override;
private:
    int mWave;
};

} // namespace levels
} // namespace oci
