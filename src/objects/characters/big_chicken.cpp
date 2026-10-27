#include "big_chicken.h"

#include "egg.h"
#include <audio/controller_holder.h>
#include <audio/player.h>
#include <context/object_storage.h>
#include <core/vector2.h>

#include <cstdlib>

namespace oci {
namespace objects {

void BigChicken::Init(const Vector2f& position, const Vector2f& destination,
                      float speed, int health) {
    Boss1::Init(position, destination, speed, health, "chicken_boss.xml");
}

void BigChicken::Fire() {
    Storage().CreateObject<audio::ControllerHolder>(audio::Play("chomp.wav"));
    for(int i = 0; i < 3; ++i) {
        const float dx = static_cast<float>((std::rand() % 120) - 60);
        Storage().CreateObject<Egg>(getPosition() + Vector2f(dx, 0));
    }
}

} // namespace objects
} // namespace oci
