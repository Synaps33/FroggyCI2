#include "wave_loader.h"

#include <core/boot_log.h>
#include <resources/path_helper.h>

#include <tinyxml2.h>

#include <cstdio>
#include <string>

namespace oci {
namespace levels {

namespace {

objects::Chicken::Type ChickenTypeFromName(const char* name) {
    if(!name)
        return objects::Chicken::tRed;
    std::string value(name);
    if(value == "orange") return objects::Chicken::tOrange;
    if(value == "yellow") return objects::Chicken::tYellow;
    if(value == "green") return objects::Chicken::tGreen;
    if(value == "cyan") return objects::Chicken::tCyan;
    if(value == "blue") return objects::Chicken::tBlue;
    if(value == "purple") return objects::Chicken::tPurple;
    if(value == "pink") return objects::Chicken::tPink;
    return objects::Chicken::tRed;
}

float ReadFloat(const tinyxml2::XMLElement* element, const char* key, float fallback) {
    float value = fallback;
    if(element->QueryFloatAttribute(key, &value) != tinyxml2::XML_SUCCESS)
        return fallback;
    return value;
}

int ReadInt(const tinyxml2::XMLElement* element, const char* key, int fallback) {
    int value = fallback;
    if(element->QueryIntAttribute(key, &value) != tinyxml2::XML_SUCCESS)
        return fallback;
    return value;
}

WaveSettings DefaultPlanet(int round) {
    static const objects::Chicken::Type types[] = {
        objects::Chicken::tRed,
        objects::Chicken::tCyan,
        objects::Chicken::tBlue,
        objects::Chicken::tYellow,
        objects::Chicken::tOrange,
        objects::Chicken::tGreen,
        objects::Chicken::tRed,
        objects::Chicken::tCyan,
        objects::Chicken::tPurple,
        objects::Chicken::tPink
    };
    WaveSettings settings;
    settings.round = round;
    settings.chicken_type = types[(round - 1) % 10];
    settings.chicken_speed = 1.8f + 0.2f * (round - 1);
    settings.flock_speed = 0.8f + 0.1f * (round - 1);
    settings.chicken_health = 1 + (round - 1) / 3;
    settings.chicken_period = 140 - 5 * (round - 1);
    settings.asteroid_count = 18 + 2 * (round - 1);
    settings.asteroid_speed = 7.0f + 0.3f * (round - 1);
    settings.paratrooper_speed = 1.8f + 0.1f * (round - 1);
    settings.paratrooper_health = 1 + (round - 1) / 3;
    settings.ufo_speed = 4.0f + 0.2f * (round - 1);
    settings.boss_speed = 3.8f + 0.2f * (round - 1);
    return settings;
}

} // namespace

WaveLoader& WaveLoader::Instance() {
    static WaveLoader instance;
    return instance;
}

WaveLoader::WaveLoader() {
    mPlanets.resize(11);
    for(int i = 1; i <= 10; ++i)
        mPlanets[i] = DefaultPlanet(i);

    const std::string path = ResolvePath("res/waves.xml");
    tinyxml2::XMLDocument doc;
    if(doc.LoadFile(path.c_str()) != tinyxml2::XML_SUCCESS) {
        BootLog("waves: cannot load %s, using defaults", path.c_str());
        return;
    }
    const tinyxml2::XMLElement* root = doc.FirstChildElement("waves");
    if(!root) {
        BootLog("waves: missing <waves> in %s", path.c_str());
        return;
    }

    for(const tinyxml2::XMLElement* planet = root->FirstChildElement("planet");
        planet; planet = planet->NextSiblingElement("planet")) {
        int index = 0;
        if(planet->QueryIntAttribute("index", &index) != tinyxml2::XML_SUCCESS ||
           index < 1 || index > 10)
            continue;
        WaveSettings settings = DefaultPlanet(index);
        settings.round = index;
        if(const char* name = planet->Attribute("name"))
            BootLog("waves: planet %d is %s", index, name);
        if(const char* chicken = planet->Attribute("chicken"))
            settings.chicken_type = ChickenTypeFromName(chicken);
        settings.chicken_speed = ReadFloat(planet, "chicken_speed", settings.chicken_speed);
        settings.flock_speed = ReadFloat(planet, "flock_speed", settings.flock_speed);
        settings.chicken_health = ReadInt(planet, "chicken_health", settings.chicken_health);
        settings.chicken_period = ReadInt(planet, "chicken_period", settings.chicken_period);
        settings.asteroid_count = ReadInt(planet, "asteroid_count", settings.asteroid_count);
        settings.asteroid_speed = ReadFloat(planet, "asteroid_speed", settings.asteroid_speed);
        settings.paratrooper_speed = ReadFloat(planet, "paratrooper_speed", settings.paratrooper_speed);
        settings.paratrooper_health = ReadInt(planet, "paratrooper_health", settings.paratrooper_health);
        settings.paratrooper_period = ReadInt(planet, "paratrooper_period", settings.paratrooper_period);
        settings.ufo_speed = ReadFloat(planet, "ufo_speed", settings.ufo_speed);
        settings.boss_speed = ReadFloat(planet, "boss_speed", settings.boss_speed);
        mPlanets[index] = settings;
    }
}

const WaveSettings& WaveLoader::GetPlanet(int round) const {
    if(round < 1) round = 1;
    if(round > 10) round = 10;
    return mPlanets[round];
}

} // namespace levels
} // namespace oci
