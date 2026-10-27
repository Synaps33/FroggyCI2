#pragma once

#include "boss1.h"

namespace oci {
namespace objects {

class MotherHen : public Boss1 {
public:
    void Init(const Vector2f& position, const Vector2f& destination,
              float speed, int health);

protected:
    void Run() override;
    void Fire() override;

private:
    int mMaxHealth;
    bool mSpawnedChickens;
};

} // namespace objects
} // namespace oci
