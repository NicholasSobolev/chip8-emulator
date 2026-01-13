#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

int main(int, char**) {
	//TESTING SDL TEMP
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window* Window = SDL_CreateWindow("Hello Window", 800, 300, 0);
  SDL_Renderer* Renderer = SDL_CreateRenderer(Window, NULL);

  bool IsRunning = true;
  SDL_Event Event;
  while (IsRunning) {
    while (SDL_PollEvent(&Event)) {
      if (Event.type == SDL_EVENT_QUIT) {
        IsRunning = false;
      }
    }
		
    SDL_SetRenderDrawColor(Renderer, 255, 0, 0, 255);  // Red background
    SDL_RenderClear(Renderer);
    SDL_RenderPresent(Renderer);
  }

  SDL_DestroyRenderer(Renderer);
  SDL_DestroyWindow(Window);
  SDL_Quit();
  return 0;
}
