#ifndef LEXGINE_OSINTERACTION_KEYBOARD_H
#define LEXGINE_OSINTERACTION_KEYBOARD_H

#include "engine/preprocessing/preprocessor_tokens.h"

namespace lexgine::osinteraction {

//! Keyboard keys recognized by the engine
enum class LEXGINE_CPP_API SystemKey
{
    tab,
    caps,
    lshift,
    lctrl,
    lalt,
    backspace,
    enter,
    keypad_enter,
    rshift,
    rctrl,
    ralt,
    esc,
    f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12,
    print_screen,
    scroll_lock,
    num_lock,
    pause,
    insert,
    home,
    page_up,
    _delete,
    end,
    page_down,
    arrow_left, arrow_right, arrow_down, arrow_up,
    space,

    num_0, num_1, num_2, num_3, num_4, num_5, num_6, num_7, num_8, num_9,
    multiply, divide, add, subtract, decimal,

    _0, _1, _2, _3, _4, _5, _6, _7, _8, _9,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    mouse_left_button, mouse_right_button, mouse_middle_button,
    mouse_x_button_1, mouse_x_button_2,

    unknown
};


}

#endif
