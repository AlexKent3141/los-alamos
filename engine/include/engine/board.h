#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace la
{

constexpr int board_side = 6;
constexpr int padded_board_side = board_side + 4;
constexpr int padded_board_area = padded_board_side * padded_board_side;

namespace eval
{
// Score which is so high it can't be attained normally.
constexpr int mate_score = 100000;
}

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

enum class MoveGenType : std::uint8_t
{
  ALL,
  DYNAMIC
};

constexpr int num_piece_types = static_cast<int>(PieceType::NUM_PIECE_TYPES);

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

namespace move
{
inline PieceType get_cap(Move m)
{ return static_cast<la::PieceType>((m & 0xFF0000) >> 16); }
inline PieceType get_promo(Move m)
{ return static_cast<PieceType>((m & 0xFF000000) >> 24); }
}

class BoardImpl;

class Board
{
public:
  Board();
  Board(const Board&);
  Board& operator=(const Board&);

  ~Board();

  Colour player_to_move() const;
  std::vector<Move> get_moves(MoveGenType type = MoveGenType::ALL) const;
  std::vector<int> get_targets_for_piece(int, int) const;
  void make_move(Move);
  void make_move(int, int, PieceType pt = PieceType::NONE);
  void undo_move(Move);
  int score() const; // The score from the current player's perspective.
  std::uint64_t hash() const;
  bool in_check() const;
  bool is_draw() const;
  std::optional<Piece> get_piece(int, int) const;

  std::string move_to_string(Move) const;

private:
  std::unique_ptr<BoardImpl> impl_;
};

}
