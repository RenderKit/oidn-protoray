// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef PRT_GUI_SUPPORT
#include <SDL2/SDL.h>
#include <GL/gl.h>
#endif

#include "sys/common.h"
#include "sys/memory.h"
#include "sys/string.h"
#include "sys/timer.h"
#include "math/vec3.h"
#include "image/surface.h"
#include "input.h"

namespace prt {

enum class DisplayMode
{
  Window,
  Fullscreen,
  Offscreen,
};

class Window : Uncopyable
{
private:
  int width;
  int height;
  DisplayMode mode;

#ifdef PRT_GUI_SUPPORT
  // SDL
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  SDL_Texture* texture = nullptr;
  SDL_Surface* screen = nullptr;
#endif

  // Text
  std::stringstream textBuffer;

  bool isRunning;
  bool isRefreshEnabled;
  bool isInputEnabled;
  Timer renderTimer;
  double renderTime;
  Timer displayTimer;
  double displayTime;
  std::string title;

public:
  Window(int width, int height, DisplayMode mode);
  virtual ~Window() {}

  // Starts the display loop
  bool run();

  int getWidth() const { return width; }
  int getHeight() const { return height; }
  DisplayMode getDisplayMode() const { return mode; }
  void setSize(int width, int height);

  const std::string& getTitle() const { return title; }
  void setTitle(const std::string& str);

  bool getInputEnabled() const { return isInputEnabled; }
  void setInputEnabled(bool flag);

  bool getRefreshEnabled() const { return isRefreshEnabled; }
  void setRefreshEnabled(bool flag);

  double getRenderTime() const { return renderTime; }
  double getDisplayTime() const { return displayTime; }

protected:
  void quit();

  // Initialization called by run() at the beginning
  virtual void onInit() {}

  // Cleanup called by run() at the end
  virtual void onDestroy() {}

  // Rendering called by run() per frame
  // Call beginFrame() and endFrame()
  virtual void onRender() {}

  // Keyboard events (called before onRender)
  virtual void onKeyDown(int key) {}
  virtual void onKeyUp(int key) {}

  // Mouse events (called before onRender)
  virtual void onMouseButtonDown(int button, int x, int y) {}
  virtual void onMouseButtonUp(int button) {}
  virtual void onMouseMotion(int x, int y, int dx, int dy) {}

  // Functions callable inside onRender()
  void beginFrame(Surface& surface);
  void endFrame();

  // Text
  std::ostream& getText()
  {
    return textBuffer;
  }

private:
  bool initDisplay();
  bool initDisplayWindow();
  bool initDisplayOffscreen();
  void destroyDisplay();

  void handleInput();
  bool getFramebufferMousePos(int& x, int& y);
};

} // namespace prt
