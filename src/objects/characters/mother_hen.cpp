#include "mother_hen.h"

#include "chicken.h"
#include "random_chicken.h"
#include <audio/controller_holder.h>
#include <audio/player.h>
#include <context/object_storage.h>
#include <core/vector2.h>
#include <objects/gun/bullet.h>
#include <utils/compat.h>

#include <cmath>

namespace oci {
namespace objects {

void MotherHen::Init(const Vector2f& position, const Vector2f& destination,
                     float speed, int health) {
    Boss1::Init(position, destination, speed, health, "mother_hen.xml");
    mMaxHealth = health;
    mSpawnedChickens = false;
}

void MotherHen::Run() {
    Boss1::Run();
    if(!mSpawnedChickens && power <= mMaxHealth / 2) {
        mSpawnedChickens = true;
        for(int i = 0; i < 8; ++i) {
            const float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / 8.0f;
            Storage().CreateObject<RandomChicken>(
                getPosition() + Vector2f(std::cos(angle) * 40.0f, std::sin(angle) * 40.0f),
                220, Chicken::tCyan, 1, 2.0f);
        }
    }
}

void MotherHen::Fire() {
    Storage().CreateObject<audio::ControllerHolder>(audio::Play("laserLow.wav"));
    for(int i = 0; i < 8; ++i) {
        const float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / 8.0f;
        Storage().CreateObject<Bullet>(
            "gun4.xml", getPosition(), 6.0f, angle, 0, 0,
            ctPlayerShip, ctEnemyBullet);
    }
}

} // namespace objects
} // namespace oci
