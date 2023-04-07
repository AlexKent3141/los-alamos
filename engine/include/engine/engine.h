#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace la
{

enum class Colour : std::uint8_t
{
  WHITE,
  BLACK
};

enum class PieceType : std::uint8_t
{
  NONE,
  PAWN,
  KNIGHT,
  ROOK,
  QUEEN,
  KING
};

// Moves are encoded:
// Byte 0: the start location index
// Byte 1: the end location index
// Byte 2: the captured piece type (can be NONE)
// Byte 3: the promotion piece type (can be NONE)
using Move = std::uint32_t;

class BoardImpl;

class Board
{
public:
  Board();
  std::vector<Move> get_moves() const;
  void make_move(Move);
  void undo_move(Move);

private:
  std::unique_ptr<BoardImpl> impl_;
};

}
