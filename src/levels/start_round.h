#pragma once

#include "level.h"
#include <background/simple_controller.h>
#include <memory>
#include <string>
#include <string_view>

namespace oci {
namespace levels {

class StartRound : public Level {
public:
    StartRound();
    void Init(std::string_view round_title);
protected:
    virtual void Run() override;
private:
    float mDY;
    int mTime;
    std::string mPlanetTitle;
    std::shared_ptr<SimpleController> mBackgroundController;
};

} // namespace levels
} // namespace oci
