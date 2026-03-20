#include <qspch.h>
#include "Layer.h"

namespace Quelos {
    Layer::Layer(const std::string& debugName)
        : m_DebugName(debugName) { }

    Layer::~Layer() { }
}
