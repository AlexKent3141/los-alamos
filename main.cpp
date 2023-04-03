#include "punk.h"
#include "SDL.h"

#include <cstdlib>
#include <stdexcept>
#include <string>

class LosAlamosApp
{
public:
  LosAlamosApp();
  ~LosAlamosApp();
  void run();

private:
  SDL_Window* window_;
  SDL_Renderer* renderer_;
};

LosAlamosApp::LosAlamosApp()
{
  static constexpr int width = 640;
  static constexpr int height = 640;

  int ret = SDL_Init(SDL_INIT_VIDEO);
  if (ret != 0)
  {
    throw std::runtime_error("Failed to initialise SDL: " + std::string(SDL_GetError()));
  }

  window_ = SDL_CreateWindow(
    "Los Alamos chess",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    width,
    height,
    0);

  if (window_ == nullptr)
  {
    throw std::runtime_error("Failed to create window: " + std::string(SDL_GetError()));
  }

  renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);

  if (renderer_ == nullptr)
  {
    throw std::runtime_error("Failed to create renderer: " + std::string(SDL_GetError()));
  }

  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
}

LosAlamosApp::~LosAlamosApp()
{
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

void LosAlamosApp::run()
{
  SDL_Event event;
  SDL_bool stop = SDL_FALSE;
  while (!stop)
  {
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_QUIT:
          stop = SDL_TRUE;
          break;
        default:
          break;
      }
    }

    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer_);

    // TODO: draw the UI.

    SDL_RenderPresent(renderer_);

    SDL_Delay(100);
  }
}

int main()
{
  LosAlamosApp app;
  app.run();

  return EXIT_SUCCESS;
}
