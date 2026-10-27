#pragma once

#include "wave_settings.h"

#include <string>
#include <vector>

namespace oci {
namespace levels {

class WaveLoader {
public:
    static WaveLoader& Instance();

    const WaveSettings& GetPlanet(int round) const;

private:
    WaveLoader();

    std::vector<WaveSettings> mPlanets;
};

} // namespace levels
} // namespace oci
