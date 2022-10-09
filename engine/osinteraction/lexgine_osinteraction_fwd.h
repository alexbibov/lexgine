#ifndef LEXGINE_OSINTERACTION_FWD_H
#define LEXGINE_OSINTERACTION_FWD_H

#include <cstdint>

namespace lexgine::osinteraction {

class AbstractListener;
template<uint32_t ... messages> class ConcreteListener;
template<typename ... ListenerTypes> class Listeners;
class WindowHandler;


}

#endif