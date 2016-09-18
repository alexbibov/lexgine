#ifndef LEXGINE_OSINTERACTION_KEYBOARD_H

namespace lexgine { namespace osinteraction {

enum class SystemKey
{
    tab,
    caps,
    lshift,
    lctrl,
    lalt,
    backspace,
    enter,
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
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z
};


}}


#define LEXGINE_OSINTERACTION_KEYBOARD_H
#endif
