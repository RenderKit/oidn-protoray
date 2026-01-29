// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <csignal>
#include "sys/constants.h"
#include "sys/logging.h"
#include "window.h"

namespace prt {

// Interrupt signal handler
namespace
{
  volatile sig_atomic_t isInterrupted = 0;

  void handleInterrupt(int signal)
  {
    isInterrupted = 1;
    Log() << "Interrupted";
  }
}

Window::Window(int width, int height, DisplayMode mode)
{
  if (width <= 0 || height <= 0)
    throw std::invalid_argument("resolution is invalid");

  this->width = width;
  this->height = height;
  this->mode = mode;
#ifdef PRT_GUI_SUPPORT
  window = 0;
#endif
  isRunning = false;
  isRefreshEnabled = true;
  isInputEnabled = mode != DisplayMode::Offscreen;
  renderTime = 0.0f;
  displayTime = posInf;
}

void Window::setSize(int width, int height)
{
  Log() << "Resolution: " << width << "x" << height;
  this->width = width;
  this->height = height;

#ifdef PRT_GUI_SUPPORT
  SDL_RenderSetLogicalSize(renderer, width, height);

  if (screen)
    SDL_FreeSurface(screen);

  screen = SDL_CreateRGBSurface(0, width, height, 32,
                                0x00FF0000,
                                0x0000FF00,
                                0x000000FF,
                                0xFF000000);

  if (texture)
    SDL_DestroyTexture(texture);

  texture = SDL_CreateTexture(renderer,
                              SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING,
                              width, height);
#endif
}

void Window::setTitle(const std::string& str)
{
  title = str;

#ifdef PRT_GUI_SUPPORT
  if (window && isRunning)
    SDL_SetWindowTitle(window, str.c_str());
#endif
}

bool Window::run()
{
  if (!initDisplay())
    return false;

  onInit();

  // Set interrupt signal handler
  isInterrupted = 0;
  signal(SIGINT, handleInterrupt);

  isRunning = true;
  Log() << "Render loop";

  // Start measuring display time
  renderTime = 0.0f;
  displayTime = posInf;
  displayTimer.reset();

  while (isRunning)
  {
    // Handle keyboard and mouse input
    handleInput();

    // Render the frame
    renderTime = 0.0f;
    displayTime = posInf;
    onRender();
  }

  onDestroy();
  destroyDisplay();

  // Restore signal handler
  signal(SIGINT, SIG_DFL);

  return true;
}

void Window::beginFrame(Surface& surface)
{
  // Map the framebuffer surface
#ifdef PRT_GUI_SUPPORT
  if (window)
  {
    surface.data   = screen->pixels;
    surface.width  = width;
    surface.height = height;
    surface.pitch  = screen->pitch;
  }
  else
#endif
  {
    surface.data = 0;
  }

  // Start measuring render time
  renderTimer.reset();
}

void Window::endFrame()
{
  // Stop measuring render time
  renderTime = renderTimer.query();

#ifdef PRT_GUI_SUPPORT
  // Unmap and draw the surface
  if (window)
  {
     // Clear framebuffer
     SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
     SDL_RenderClear(renderer);

     // Draw frame
     SDL_UpdateTexture(texture, nullptr, screen->pixels, screen->pitch);
     SDL_RenderCopy(renderer, texture, nullptr, nullptr);

     SDL_RenderPresent(renderer);
  }
#endif

  // Clear the text buffer
  textBuffer.str(std::string());

  // Stop measuring display time
  displayTime = displayTimer.query();
  displayTimer.reset();
}

void Window::quit()
{
  isRunning = false;
}

bool Window::initDisplay()
{
  if (mode == DisplayMode::Offscreen)
    return initDisplayOffscreen();

  return initDisplayWindow();
}

bool Window::initDisplayWindow()
{
#ifdef PRT_GUI_SUPPORT
  Log() << "Creating window";

  if (SDL_Init(SDL_INIT_VIDEO) != 0)
  {
    LogError() << "Could not initialize SDL";
    return false;
  }

  Uint32 windowFlags = 0;
  if (mode == DisplayMode::Fullscreen)
    windowFlags |= SDL_WINDOW_FULLSCREEN;
  else
    windowFlags |= SDL_WINDOW_RESIZABLE;
  window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, windowFlags);

  if (window == 0)
  {
    LogError() << "Could not set video mode";
    return false;
  }

  renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"); // linear

  setSize(width, height);
  return true;
#else
  LogError() << "GUI not supported";
  return false;
#endif
}

// Initialize an offscreen framebuffer
bool Window::initDisplayOffscreen()
{
  Log() << "Offscreen mode";

#ifdef PRT_GUI_SUPPORT
  window = 0;
#endif
  return true;
}

void Window::destroyDisplay()
{
#ifdef PRT_GUI_SUPPORT
  if (window)
  {
    Log() << "Destroying window";

    if (texture)
      SDL_DestroyTexture(texture);
    if (screen)
      SDL_FreeSurface(screen);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
  }
#endif
}

void Window::handleInput()
{
  if (isInterrupted)
  {
    quit();
    return;
  }

#ifdef PRT_GUI_SUPPORT
  if (!window)
    return;

  SDL_Event event;
  int mouseX, mouseY;
  int mouseDx, mouseDy;
  int mouseButton;

  while (SDL_PollEvent(&event))
  {
    if (!isInputEnabled)
      continue;

    switch (event.type)
    {
    case SDL_KEYDOWN:
      onKeyDown(event.key.keysym.sym);
      break;

    case SDL_KEYUP:
      onKeyUp(event.key.keysym.sym);
      break;

    case SDL_MOUSEBUTTONDOWN:
      if (getFramebufferMousePos(mouseX, mouseY))
      {
        switch (event.button.button)
        {
        case SDL_BUTTON_LEFT:   mouseButton = MouseButton::Left;   break;
        case SDL_BUTTON_RIGHT:  mouseButton = MouseButton::Right;  break;
        case SDL_BUTTON_MIDDLE: mouseButton = MouseButton::Middle; break;
        default:                mouseButton = MouseButton::Other;  break;
        }
        onMouseButtonDown(mouseButton, mouseX, mouseY);
      }
      break;

    case SDL_MOUSEBUTTONUP:
      onMouseButtonUp(event.button.button);
      break;

    case SDL_MOUSEWHEEL:
      if (getFramebufferMousePos(mouseX, mouseY))
      {
        if (event.wheel.y > 0)
          onMouseButtonDown(MouseButton::WheelUp, mouseX, mouseY);
        else
          onMouseButtonDown(MouseButton::WheelDown, mouseX, mouseY);
      }
      break;

    case SDL_MOUSEMOTION:
      if (getFramebufferMousePos(mouseX, mouseY))
      {
        SDL_GetRelativeMouseState(&mouseDx, &mouseDy);
        onMouseMotion(mouseX, mouseY, mouseDx, mouseDy);
      }
      break;

    case SDL_QUIT:
      quit();
      break;
    }
  }
#endif
}

bool Window::getFramebufferMousePos(int& x, int& y)
{
#ifdef PRT_GUI_SUPPORT
  if (!window)
    return false;

  // Get the mouse position in framebuffer coords, taking into account window size and letterboxing
  int winW, winH;
  SDL_GetWindowSize(window, &winW, &winH);

  const float aspectWin = float(winW) / float(winH);
  const float aspectFb  = float(width) / float(height);
  int fbX = 0;
  int fbY = 0;
  int fbW = winW;
  int fbH = winH;
  if (aspectWin > aspectFb)
  {
    // Window is wider than framebuffer
    fbW = int(aspectFb * float(winH));
    fbX = (winW - fbW) / 2;
  }
  else if (aspectWin < aspectFb)
  {
    // Window is taller than framebuffer
    fbH = int(float(winW) / aspectFb);
    fbY = (winH - fbH) / 2;
  }

  int mouseX, mouseY;
  SDL_GetMouseState(&mouseX, &mouseY);
  if (mouseX < fbX || mouseX >= fbX + fbW || mouseY < fbY || mouseY >= fbY + fbH)
    return false;

  const int posX = (mouseX - fbX) * width / fbW;
  const int posY = (mouseY - fbY) * height / fbH;
  if (posX < 0 || posX >= width || posY < 0 || posY >= height)
    return false;
  x = posX;
  y = posY;
  return true;
#else
  return false;
#endif
}

void Window::setInputEnabled(bool flag)
{
  isInputEnabled = flag;

  if (isInputEnabled)
    Log() << "Input: enabled";
  else
    Log() << "Input: disabled";
}

void Window::setRefreshEnabled(bool flag)
{
  isRefreshEnabled = flag;

  if (isRefreshEnabled)
    Log() << "Refresh: enabled";
  else
    Log() << "Refresh: disabled";
}

} // namespace prt
