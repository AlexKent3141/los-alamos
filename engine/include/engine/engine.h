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
  PAWN_WHITE,
  PAWN_BLACK,
  KNIGHT,
  ROOK,
  QUEEN,
  KING,
  NUM_PIECE_TYPES
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
  void make_move(int, int, PieceType pt = PieceType::NONE);
  void undo_move(Move);
  int score() const; // The score from the current player's perspective.
  std::optional<Piece> get_piece(int, int) const;

private:
  std::unique_ptr<BoardImpl> impl_;
};

}
