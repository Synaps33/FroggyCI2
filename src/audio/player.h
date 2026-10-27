#pragma once

#include "controller.h"
#include <cstdint>
#include <cstddef>
#include <memory>
#include <string_view>

namespace oci {
namespace audio {

std::unique_ptr<Controller> Play(std::string_view filename,
                                 bool autoplay = true,
                                 bool loop = false);

void AudioMix(int16_t* out_stereo, size_t frames);

/// Immediately silences every voice (menu, new game, unload).
void StopAll();
/// Stops only looping voices (e.g. UFO engine on game over).
void StopLoops();

} // namespace audio
} // namespace oci
