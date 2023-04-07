#include "SDL.h"

extern "C"
{
#include "punk.h"
}

#include <array>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>

enum class PieceColour
{
  WHITE,
  BLACK
};

enum class PieceType
{
  PAWN,
  KNIGHT,
  ROOK,
  QUEEN,
  KING
};

struct Piece
{
  PieceColour colour;
  PieceType type;

  std::string image_path() const;
};

std::string Piece::image_path() const
{
  std::string colour_str, type_str;
  switch (colour)
  {
    case PieceColour::WHITE: colour_str = "white"; break;
    case PieceColour::BLACK: colour_str = "black"; break;
  }
  switch (type)
  {
    case PieceType::PAWN:   type_str = "pawn";   break;
    case PieceType::KNIGHT: type_str = "knight"; break;
    case PieceType::ROOK:   type_str = "rook";   break;
    case PieceType::QUEEN:  type_str = "queen";  break;
    case PieceType::KING:   type_str = "king";   break;
  }

  return "res/" + colour_str + "_" + type_str + ".png";
}

class ChessState
{
public:
  ChessState();
  std::optional<Piece> get_piece(int, int) const;

private:
  // The piece at index 0 is white's queenside rook.
  std::array<std::optional<Piece>, 36> pieces_;
};

ChessState::ChessState()
{
  pieces_.fill(std::nullopt);

  static constexpr std::array<PieceType, 6> backrank =
  {
    PieceType::ROOK,
    PieceType::KNIGHT,
    PieceType::QUEEN,
    PieceType::KING, 
    PieceType::KNIGHT,
    PieceType::ROOK
  };

  for (int c = 0; c < 6; c++)
  {
    pieces_[c] = { PieceColour::WHITE, backrank[c] };
    pieces_[6 + c] = { PieceColour::WHITE, PieceType::PAWN };
    pieces_[24 + c] = { PieceColour::BLACK, PieceType::PAWN };
    pieces_[30 + c] = { PieceColour::BLACK, backrank[c] };
  }
}

std::optional<Piece> ChessState::get_piece(int row, int col) const
{
  return pieces_[6 * row + col];
}

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

  if (SDL_Init(SDL_INIT_VIDEO) != 0)
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

  punk_init(renderer_, width, height);
}

LosAlamosApp::~LosAlamosApp()
{
  punk_quit();

  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

void LosAlamosApp::run()
{
  struct punk_style black_square, white_square;
  punk_default_style(&black_square);
  black_square.back_colour_rgba = 0x0000FFFF;
  black_square.control_colour_rgba = 0x0000FFFF;
  black_square.active_colour_rgba = 0x7070FFFF;
  punk_default_style(&white_square);
  white_square.back_colour_rgba = 0xFFFFFFFF;
  white_square.control_colour_rgba = 0xFFFFFFFF;
  white_square.active_colour_rgba = 0x7070FFFF;

  ChessState state;

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

      punk_handle_event(&event);
    }

    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer_);

    punk_begin();

    punk_begin_vertical_layout("1:1:1:1:1:1", PUNK_FILL, PUNK_FILL);
    for (int r = 0; r < 6; r++)
    {
      int sq = r & 0x1;

      punk_begin_horizontal_layout("1:1:1:1:1:1", PUNK_FILL, PUNK_FILL);
      for (int c = 0; c < 6; c++)
      {
        const auto piece = state.get_piece(5 - r, c);
        const auto style = (sq++ & 0x1) ? white_square : black_square;
        if (!piece)
        {
          punk_label(" ", &style);
        }
        else
        {
          punk_picture_button(piece->image_path().c_str(), &style);
        }
      }

      punk_end_layout();
    }

    punk_end_layout();

    punk_end();

    punk_render();

    SDL_RenderPresent(renderer_);

    SDL_Delay(50);
  }
}

int main()
{
  LosAlamosApp app;
  app.run();

  return EXIT_SUCCESS;
}
