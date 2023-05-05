#pragma once

#include "engine/board.h"

namespace la
{

// Store a static set of Zobrist keys.
struct keys
{
  keys();

  static std::uint64_t white_key;
  static std::uint64_t piece_square_keys[2][num_piece_types][padded_board_area];
};

}
