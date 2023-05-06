#include "SDL.h"

extern "C"
{
#include "punk.h"
}

#include "engine/engine.h"
#include "search/search.h"

#include <array>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace la;

std::chrono::milliseconds search_time{1000};

static std::string image_path(const Piece& piece)
{
  std::string colour_str, type_str;
  switch (piece.colour)
  {
    case Colour::WHITE: colour_str = "white"; break;
    case Colour::BLACK: colour_str = "black"; break;
  }
  switch (piece.type)
  {
    case PieceType::PAWN_WHITE:
    case PieceType::PAWN_BLACK: type_str = "pawn";   break;
    case PieceType::KNIGHT: type_str = "knight"; break;
    case PieceType::ROOK:   type_str = "rook";   break;
    case PieceType::QUEEN:  type_str = "queen";  break;
    case PieceType::KING:   type_str = "king";   break;
  }

  return "res/" + colour_str + "_" + type_str + ".png";
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
  static constexpr int width = 1280;
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
  struct punk_style black_square, white_square, piece_target;

  punk_default_style(&black_square);
  black_square.back_colour_rgba = 0x0000FFFF;
  black_square.control_colour_rgba = 0x0000FFFF;
  black_square.active_colour_rgba = 0x7070FFFF;

  punk_default_style(&white_square);
  white_square.back_colour_rgba = 0xFFFFFFFF;
  white_square.control_colour_rgba = 0xFFFFFFFF;
  white_square.active_colour_rgba = 0x7070FFFF;

  punk_default_style(&piece_target);
  piece_target.back_colour_rgba = 0xFF7070FF;
  piece_target.control_colour_rgba = 0xFF7070FF;

  Board state;
  int selected_index, target_index;
  Piece moving_piece;
  std::set<int> piece_targets;

  std::mutex search_result_mutex;
  std::vector<std::pair<SearchData, std::string>> search_results;

  const auto data_to_row = [&] (const SearchData& data) -> std::string
  {
    char buf[1024];
    std::sprintf(
      buf,
      "%6d %5s %6d",
      data.depth,
      state.move_to_string(data.best_move).c_str(),
      data.score);
    return buf;
  };

  auto search_result_callback = [&] (const SearchData& search_data)
  {
    std::lock_guard lock(search_result_mutex);
    search_results.push_back(std::make_pair(search_data, data_to_row(search_data)));
  };

  SearchWorker search_worker(search_result_callback);

  enum class Screen
  {
    BOARD,
    SELECT_PROMOTION
  };

  auto screen = Screen::BOARD;
  PieceType promotion_type;

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

    if (screen == Screen::BOARD)
    {
      punk_begin_horizontal_layout("1:1", PUNK_FILL, PUNK_FILL);

      punk_begin_vertical_layout("1:1:1:1:1:1", PUNK_FILL, PUNK_FILL);
      for (int r = 5; r >= 0; r--)
      {
        int sq = r & 0x1;

        punk_begin_horizontal_layout("1:1:1:1:1:1", PUNK_FILL, PUNK_FILL);
        for (int c = 0; c < 6; c++)
        {
          bool is_target = piece_targets.find(6*r + c) != std::end(piece_targets);
          const auto piece = state.get_piece(r, c);
          punk_style style;
          if (is_target)
          {
            style = piece_target;
          }
          else
          {
            style = (sq & 0x1) ? white_square : black_square;
          }

          ++sq;

          if (!piece)
          {
            if (!is_target)
            {
              // Unclickable empty location.
              punk_label(" ", &style);
            }
            else
            {
              // Clickable empty location.
              if (punk_button(" ", &style))
              {
                piece_targets.clear();
                target_index = 6*r + c;
                if ((moving_piece.type == PieceType::PAWN_WHITE && r == 5) ||
                    (moving_piece.type == PieceType::PAWN_BLACK && r == 0))
                {
                  screen = Screen::SELECT_PROMOTION;
                }
                else
                {
                  search_results.clear();
                  state.make_move(selected_index, target_index);
                }
              }
            }
          }
          else
          {
            if (punk_picture_button(image_path(*piece).c_str(), &style))
            {
              if (is_target)
              {
                // Actually make the move.
                piece_targets.clear();
                target_index = 6*r + c;
                if ((moving_piece.type == PieceType::PAWN_WHITE && r == 5) ||
                    (moving_piece.type == PieceType::PAWN_BLACK && r == 0))
                {
                  screen = Screen::SELECT_PROMOTION;
                }
                else
                {
                  search_results.clear();
                  state.make_move(selected_index, target_index);
                }
              }
              else
              {
                // Selected a new piece.
                selected_index = 6*r + c;
                moving_piece = *piece;

                // Store the targets.
                const auto targets = state.get_targets_for_piece(r, c);
                piece_targets.clear();
                for (const auto target : targets)
                {
                  piece_targets.insert(target);
                }
              }
            }
          }
        }

        punk_end_layout();
      }

      punk_end_layout();

      // This is the right-hand side of the screen.
      // Buttons along the top and search results vertically below that.
      punk_begin_vertical_layout("e40:e40:1", PUNK_FILL, PUNK_FILL);

      if (search_worker.running())
      {
        punk_label("Searching...", NULL);
      }
      else
      {
        if (punk_button("Computer move", NULL))
        {
          search_results.clear();
          search_worker.start(state, search_time);
        }
      }

      // Column headers.
      punk_label("Depth | Move | Score", NULL);

      {
        if (!search_results.empty())
        {
          std::lock_guard lock(search_result_mutex);

          // Display the last 8 results.
          punk_begin_vertical_layout("1:1:1:1:1:1:1:1", PUNK_FILL, PUNK_FILL);
          std::size_t start = search_results.size() > 8 ? search_results.size() - 8 : 0;
          for (std::size_t i = start; i < search_results.size(); i++)
          {
            punk_label(search_results[i].second.c_str(), NULL);
          }

          for (std::size_t i = search_results.size(); i < 8; i++)
          {
            punk_skip_layout_widget();
          }

          punk_end_layout();
        }
        else
        {
          punk_skip_layout_widget();
        }
      }

      punk_end_layout();

      punk_end_layout();
    }
    else
    {
      // Display pawn promotion options.
      punk_begin_vertical_layout("1:1:1", PUNK_FILL, PUNK_FILL);

      punk_label("Pick a promotion type:", NULL);

      punk_begin_horizontal_layout("1:1:1", PUNK_FILL, PUNK_FILL);
      Piece promo_piece { moving_piece.colour, PieceType::NONE };
      for (const auto pt : { PieceType::KNIGHT, PieceType::ROOK, PieceType::QUEEN })
      {
        promo_piece.type = pt;
        if (punk_picture_button(image_path(promo_piece).c_str(), NULL))
        {
          // We now know the promotion type so can make the move.
          state.make_move(selected_index, target_index, pt);
          screen = Screen::BOARD;
        }
      }

      punk_end_layout();

      punk_skip_layout_widget();

      punk_end_layout();
    }

    punk_end();

    punk_render();

    SDL_RenderPresent(renderer_);

    SDL_Delay(50);
  }
}

int main(int argc, char** argv)
{
  if (argc > 1)
  {
    search_time = std::chrono::milliseconds(std::stoi(argv[1]));
  }

  LosAlamosApp app;
  app.run();

  return EXIT_SUCCESS;
}
