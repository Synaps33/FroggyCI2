#include "wave_settings.h"

namespace oci {
namespace levels {

namespace {
WaveSettings s_current;
}

WaveSettings& CurrentWave() {
    return s_current;
}

const char* WaveName(int round, int wave, const char* fallback) {
    static const char* names[10][10] = {
        { "first contact", "classic invasion", "one more time", "meteor shower",
          "paratroopers!", "death from above", "third time lucky", "asteroids!",
          "bonus", "show'em who's boss!" },
        { "deep freeze", "blue formation", "cold front", "ice shower",
          "frozen troops", "snowed under", "last snowflake", "comet storm",
          "lost signal", "neptune's wrath" },
        { "sideways planet", "tilted wings", "rolling thunder", "rock rain",
          "umbrella corps", "falling skies", "no way out", "ring fragments",
          "strange signal", "titan of ice" },
        { "ring race", "golden flock", "ring formation", "meteor belt",
          "ring paratroopers", "saturn's shadow", "crushed ice", "asteroid rings",
          "gift from space", "lord of the rings" },
        { "great red spot", "giant flock", "storm front", "rock hurricane",
          "gas giants", "pressure drop", "eye of the storm", "asteroid storm",
          "deep space bonus", "king of planets" },
        { "rocky road", "stone formation", "boulder dash", "meteor storm",
          "asteroid troops", "raining rocks", "no safe place", "asteroid field",
          "lost treasure", "belt buckle" },
        { "red planet", "red wings", "dust storm", "red rain",
          "martian troops", "thin atmosphere", "last stand", "red rocks",
          "alien signal", "god of war" },
        { "home sweet home", "blue planet", "final approach", "city lights",
          "defenders", "cloud cover", "last hope", "meteor shower",
          "orbital bonus", "save the world" },
        { "morning star", "hot wings", "acid clouds", "burning rain",
          "venusian guard", "heat wave", "no escape", "lava rocks",
          "golden signal", "goddess of love" },
        { "scorched planet", "fast flock", "solar wind", "meteor blaze",
          "last line", "heat stroke", "final countdown", "sun rocks",
          "last bonus", "final boss" }
    };
    if(round < 1 || round > 10 || wave < 1 || wave > 10)
        return fallback;
    return names[round - 1][wave - 1];
}

} // namespace levels
} // namespace oci
