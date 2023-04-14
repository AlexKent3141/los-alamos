#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace la
{

enum class Colour : std::uint8_t
{
  WHITE = 0,
  BLACK = 1
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

struct Piece
{
  Colour colour;
  PieceType type;
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
  ~Board();
  Colour player_to_move() const;
  std::vector<Move> get_moves() const;
  std::vector<int> get_targets_for_piece(int, int) const;
  void make_move(Move);
  void make_move(int, int);
  void undo_move(Move);
  std::optional<Piece> get_piece(int, int) const;

private:
  std::unique_ptr<BoardImpl> impl_;
};

}
