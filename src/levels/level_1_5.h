#pragma once

#include "game_level.h"
#include <objects/base/object.h>
#include <forward_list>
#include <memory>

namespace oci {
namespace levels {

class CLevel_1_5 : public GameLevel {
public:
    void Init(int wave);
protected:
    virtual void Run() override;
private:
    inline void CreateParatrooper(float x, float y);

    typedef std::forward_list<std::weak_ptr<objects::Object> > ChickensList;
    ChickensList mChickens;
    int mWave;
};

} // namespace levels
} // namespace oci
