// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef PRT_GUI_SUPPORT
#include <SDL2/SDL_keycode.h>
#endif

namespace prt {

namespace Key
{
  enum : int
  {
  #ifdef PRT_GUI_SUPPORT
    Return      = SDLK_RETURN,
    Escape      = SDLK_ESCAPE,
    Backspace   = SDLK_BACKSPACE,
    Left        = SDLK_LEFT,
    Right       = SDLK_RIGHT,
    Up          = SDLK_UP,
    Down        = SDLK_DOWN,
    NumpadPlus  = SDLK_KP_PLUS,
    NumpadMinus = SDLK_KP_MINUS,
    Home        = SDLK_HOME,
    End         = SDLK_END,
    PageUp      = SDLK_PAGEUP,
    PageDown    = SDLK_PAGEDOWN,
  #else
    Return,
    Escape,
    Backspace,
    Left,
    Right,
    Up,
    Down,
    NumpadPlus,
    NumpadMinus,
    Home,
    End,
    PageUp,
    PageDown,
  #endif
  };
}

namespace MouseButton
{
  enum : int
  {
    Other,
    Left,
    Right,
    Middle,
    WheelUp,
    WheelDown
  };
};

} // namespace prt
