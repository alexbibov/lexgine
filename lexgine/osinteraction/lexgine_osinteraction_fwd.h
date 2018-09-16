#ifndef LEXGINE_OSINTERACTION_FWD_H
#define LEXGINE_OSINTERACTION_FWD_H

namespace lexgine { namespace osinteraction {

class AbstractListener;
template<uint32_t ... messages> class ConcreteListener;
template<typename ... ListenerTypes> class Listeners;

}}

#endif