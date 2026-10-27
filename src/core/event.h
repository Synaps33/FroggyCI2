#pragma once

namespace oci {

class Event {
public:
    enum EventType {
        None = 0,
        Closed = 1
    };

    EventType type = None;
};

} // namespace oci
