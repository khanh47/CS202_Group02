#pragma once

#include <box2d/box2d.h>

class WorldInteractionSystem {
public:
    void processContacts(b2ContactEvents events);
    void processSensors(b2SensorEvents events);
};
