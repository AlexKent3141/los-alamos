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

bool on_board(Square sq) { return sq & 0xFF; }
PieceType get_pt(Square sq) { return static_cast<PieceType>((sq & 0xFF00) >> 8); }
Colour get_colour(Square sq) { return static_cast<Colour>((sq & 0xFF0000) >> 16); }

}

namespace la
{

static Move create_move(
  int start,
  int end,
  PieceType cap = PieceType::NONE,
  PieceType promotion = PieceType::NONE)
{
  return start + (end << 8) + (static_cast<int>(cap) << 16) + (static_cast<int>(promotion) << 24);
}

// Represent the board state using a "letter-box" style structure.
class BoardImpl
{
public:
  BoardImpl();
  std::vector<Move> get_moves() const;
  std::vector<int> get_targets_for_piece(int, int) const;
  Colour player_to_move() const { return player_to_move_; }
  void make_move(Move);
  void make_move(int, int);
  void undo_move(Move);
  std::optional<Piece> get_piece(int, int) const;

private:
  static constexpr int board_side = 6;
  static constexpr int padded_board_side = board_side + 4;
  static constexpr int padded_board_area = padded_board_side * padded_board_side;

  // Padded offsets for each piece's moves.
  static constexpr std::array<std::array<int, 8>, 6> piece_offsets =
  {
    std::array<int, 8>
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    std::array<int, 8>
    { padded_board_side, 0, 0, 0, 0, 0, 0, 0 },
    std::array<int, 8>
    { 2 * padded_board_side + 1, 2 * padded_board_side - 1,
      padded_board_side + 2, padded_board_side - 2,
      -padded_board_side + 2, -padded_board_side - 2,
      -2 * padded_board_side + 1, -2 * padded_board_side - 1 },
    std::array<int, 8>
    { -1, 1, padded_board_side, -padded_board_side, 0, 0, 0, 0 },
    std::array<int, 8>
    { -1, 1, padded_board_side, -padded_board_side,
      padded_board_side - 1, padded_board_side + 1, -padded_board_side - 1, -padded_board_side + 1 },
    std::array<int, 8>
    { -1, 1, padded_board_side, -padded_board_side,
      padded_board_side - 1, padded_board_side + 1, -padded_board_side - 1, -padded_board_side + 1 }
  };

  Colour player_to_move_;
  std::array<Square, padded_board_area> squares_;

  static constexpr int to_padded(int r, int c) const
  {
    return (r + 2) * padded_board_side + c + 2;
  }

  static constexpr int to_padded(int loc) const
  {
    return to_padded(loc / board_side, loc % board_side);
  }

  static constexpr int from_padded(int loc) const
  {
    int padded_row = loc / padded_board_side;
    int padded_col = loc % padded_board_side;
    return (padded_row - 2) * board_side + padded_col - 2;
  }
};

BoardImpl::BoardImpl() : player_to_move_(Colour::WHITE)
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
      auto& sq = squares_[to_padded(r, c)];
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
    set_square_properties(squares_[to_padded(0, c)], Colour::WHITE, backrank[c]);
    set_square_properties(squares_[to_padded(1, c)], Colour::WHITE, PieceType::PAWN);
    set_square_properties(squares_[to_padded(4, c)], Colour::BLACK, PieceType::PAWN);
    set_square_properties(squares_[to_padded(5, c)], Colour::BLACK, backrank[c]);
  }
}

std::vector<Move> BoardImpl::get_moves() const
{
  // TODO
  return std::vector<Move>();
}

std::vector<int> BoardImpl::get_targets_for_piece(int row, int col) const
{
  // TODO: Finish implementing pawn moves and checks etc.
  int loc = to_padded(row, col);
  const Square sq = squares_[loc];
  assert(sq & 0xFF); // Not part of the padding.
  const auto pt = static_cast<PieceType>((sq & 0xFF00) >> 8);
  assert(pt != PieceType::NONE); // The square must be occupied.

  // Does the player to move own this piece?
  std::vector<int> targets;
  if (get_col(sq) != player_to_move_)
  {
    return targets;
  }

  const auto& offsets = piece_offsets[static_cast<int>(pt)];

  switch (pt)
  {
    case PieceType::PAWN:
      if (player_to_move_ == Colour::WHITE)
      {
        targets.push_back(from_padded(loc + padded_board_side));
      }
      else
      {
        targets.push_back(from_padded(loc - padded_board_side));
      }
      break;
    case PieceType::KNIGHT: case PieceType::KING:
      for (const auto offset : offsets)
      {
        if (offset == 0) break;
        int target = loc + offset;
        const auto target_sq = squares_[target];
        if (!on_board(target_sq)) continue;
        if (get_pt(target_sq) != PieceType::NONE &&
            get_col(target_sq) == player_to_move_) continue;
        targets.push_back(from_padded(target));
      }
      break;
    case PieceType::ROOK: case PieceType::QUEEN:
      for (const auto offset : offsets)
      {
        if (offset == 0) break;
        int target = loc + offset;
        auto target_sq = squares_[target];
        while (on_board(target_sq))
        {
          if (get_pt(target_sq) != PieceType::NONE &&
              get_col(target_sq) == player_to_move_) continue;
          targets.push_back(from_padded(target));
          target += offset;
          target_sq = squares_[target];
        }
      }
      break;
  }

  return targets;
}

void BoardImpl::make_move(Move move)
{
  int start = move & 0xFF;
  int end = (move >> 8) & 0xFF;

  auto& start_sq = squares_[start];
  auto& end_sq = squares_[end];

  const int moving_piece_type = start_sq & 0xFF00;
  start_sq &= ~0xFFFF00;
  end_sq &= ~0xFF00;
  end_sq |= moving_piece_type;

  end_sq &= 0x00FFFF;
  end_sq |= static_cast<int>(player_to_move_) << 16;

  player_to_move_ = player_to_move_ == Colour::WHITE ? Colour::BLACK : Colour::WHITE;
}

void BoardImpl::make_move(int start, int end)
{
  // TODO: Implement promotions.
  // Create a move.
  // Is there a capture?
  const auto& sq = squares_[to_padded(end)];
  auto cap = static_cast<PieceType>((sq & 0xFF00) >> 8);
  Move move = create_move(to_padded(start), to_padded(end), cap);
  make_move(move);
}

void BoardImpl::undo_move(Move move)
{
  // TODO
}

std::optional<Piece> BoardImpl::get_piece(int row, int col) const
{
  assert(row >= 0 && row < board_side && col >= 0 && col < board_side);

  Square sq = squares_[to_padded(row, col)];

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

Colour Board::player_to_move() const
{
  return impl_->player_to_move();
}

std::vector<Move> Board::get_moves() const
{
  return impl_->get_moves();
}

std::vector<int> Board::get_targets_for_piece(int row, int col) const
{
  return impl_->get_targets_for_piece(row, col);
}

void Board::make_move(Move move)
{
  impl_->make_move(move);
}

void Board::make_move(int start, int end)
{
  impl_->make_move(start, end);
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
