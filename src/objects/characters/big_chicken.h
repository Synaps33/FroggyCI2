#pragma once

#include "boss1.h"

namespace oci {
namespace objects {

class BigChicken : public Boss1 {
public:
    void Init(const Vector2f& position, const Vector2f& destination,
              float speed, int health);

protected:
    void Fire() override;
};

} // namespace objects
} // namespace oci
