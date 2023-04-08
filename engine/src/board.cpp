#include "engine/engine.h"

#include <array>
#include <cassert>

namespace
{

// Encode the data for a square.
// Byte 0: 0 if the square is part of the padding
// Byte 1: the piece type (can be NONE)
// Byte 2: the colour of the piece
using Square = std::uint32_t;

}

namespace la
{

// Represent the board state using a "letter-box" style structure.
class BoardImpl
{
public:
  BoardImpl();
  std::vector<Move> get_moves() const;
  void make_move(Move);
  void undo_move(Move);
  std::optional<Piece> get_piece(int, int) const;

private:
  static constexpr int board_side = 6;
  static constexpr int padded_board_side = board_side + 4;
  static constexpr int padded_board_area = padded_board_side * padded_board_side;

  std::array<Square, padded_board_area> squares_;

  static constexpr auto padded_loc = [] (int r, int c)
  {
    return (r + 2) * padded_board_side + c + 2;
  };

};

BoardImpl::BoardImpl()
{
  // Start off by marking all squares as being part of the padding.
  squares_.fill(0);

  static constexpr auto set_square_properties = [] (Square& sq, Colour col, PieceType pt)
  {
    sq |= static_cast<std::uint8_t>(pt) << 8;
    sq |= static_cast<std::uint8_t>(col) << 16;
  };

  // Make a pass where we mark squares that are on the board.
  for (int r = 0; r < board_side; r++)
  {
    for (int c = 0; c < board_side; c++)
    {
      auto& sq = squares_[padded_loc(r, c)];
      sq |= 0x1;
    }
  }

  // Now do the initial piece setup.
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
    set_square_properties(squares_[padded_loc(0, c)], Colour::WHITE, backrank[c]);
    set_square_properties(squares_[padded_loc(1, c)], Colour::WHITE, PieceType::PAWN);
    set_square_properties(squares_[padded_loc(4, c)], Colour::BLACK, PieceType::PAWN);
    set_square_properties(squares_[padded_loc(5, c)], Colour::BLACK, backrank[c]);
  }
}

std::vector<Move> BoardImpl::get_moves() const
{
  // TODO
  return std::vector<Move>();
}

void BoardImpl::make_move(Move move)
{
  // TODO
}

void BoardImpl::undo_move(Move move)
{
  // TODO
}

std::optional<Piece> BoardImpl::get_piece(int row, int col) const
{
  assert(row >= 0 && row < board_side && col >= 0 && col < board_side);

  Square sq = squares_[padded_loc(row, col)];

  std::uint8_t pt = (sq & 0xFF00) >> 8;
  if (!pt)
  {
    // No piece here.
    return std::nullopt;
  }

  std::uint8_t colour = (sq & 0xFF0000) >> 16;
  return Piece { static_cast<Colour>(colour), static_cast<PieceType>(pt) };
}

Board::Board() : impl_(std::make_unique<BoardImpl>())
{
}

Board::~Board() = default;

std::vector<Move> Board::get_moves() const
{
  return impl_->get_moves();
}

void Board::make_move(Move move)
{
  impl_->make_move(move);
}

void Board::undo_move(Move move)
{
  impl_->undo_move(move);
}

std::optional<Piece> Board::get_piece(int row, int col) const
{
  return impl_->get_piece(row, col);
}

}
