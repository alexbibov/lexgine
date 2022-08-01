#ifndef LEXGINE_OSINTERACTION_MOUSE_H
#define LEXGINE_OSINTERACTION_MOUSE_H

#include <engine/preprocessing/preprocessor_tokens.h>
#include <engine/core/misc/flags.h>

namespace lexgine::osinteraction {

//! Flags representing virtual control keys that may affect mouse actions when pressed
LEXGINE_CPP_API BEGIN_FLAGS_DECLARATION(ControlKeyFlag)
FLAG(ctrl, 1)
FLAG(left_mouse_button, 2)
FLAG(middle_mouse_button, 4)
FLAG(right_mouse_button, 8)
FLAG(shift, 0x10)
FLAG(xbutton1, 0x20)
FLAG(xbutton2, 0x40)
END_FLAGS_DECLARATION(ControlKeyFlag);


//! Enumerates basic three mouse buttons
enum class LEXGINE_CPP_API MouseButton
{
    left,
    middle,
    right,
    x
};

}

#endif
