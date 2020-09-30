#include "listener.h"
#include <cassert>

using namespace lexgine::osinteraction;



int64_t AbstractListener::handle(uint64_t message, uint64_t param0, uint64_t param1, uint64_t param2, uint64_t param3,
    uint64_t param4, uint64_t param5, uint64_t param6, uint64_t param7)
{
    if (doesHandle(message)) return process_message(message, param0, param1, param2, param3, param4, param5, param6, param7);

    return not_supported;
}

bool AbstractListener::doesHandle(uint64_t message) const
{
    assert(false);
    return false;
}

int64_t AbstractListener::process_message(uint64_t message, uint64_t param0, uint64_t param1, uint64_t param2, uint64_t param3,
    uint64_t param4, uint64_t param5, uint64_t param6, uint64_t param7)
{
    assert(false);
    return not_supported;
}
