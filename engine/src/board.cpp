#include "engine/engine.h"

#include <algorithm>
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

namespace square
{

bool on_board(Square sq) { return sq & 0xFF; }
la::PieceType get_pt(Square sq) { return static_cast<la::PieceType>((sq & 0xFF00) >> 8); }
la::Colour get_colour(Square sq) { return static_cast<la::Colour>((sq & 0xFF0000) >> 16); }

}

namespace move
{

int get_start(la::Move m) { return m & 0xFF; }
int get_end(la::Move m) { return (m & 0xFF00) >> 8; }
la::PieceType get_cap(la::Move m) { return static_cast<la::PieceType>((m & 0xFF0000) >> 16); }
la::PieceType get_promo(la::Move m) { return static_cast<la::PieceType>((m & 0xFF000000) >> 24); }

la::Move create(
  int start,
  int end,
  la::PieceType cap = la::PieceType::NONE,
  la::PieceType promotion = la::PieceType::NONE)
{
  return start + (end << 8) + (static_cast<int>(cap) << 16) + (static_cast<int>(promotion) << 24);
}

}

namespace la
{

// Represent the board state using a "letter-box" style structure.
class BoardImpl
{
public:
  BoardImpl();
  std::vector<Move> get_moves() const;
  std::vector<int> get_targets_for_piece(int, int) const;
  Colour player_to_move() const { return player_to_move_; }
  void make_move(Move);
  void make_move(int, int, PieceType);
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
  mutable std::array<Square, padded_board_area> squares_;
  std::array<int, 2> king_locations_;

  static constexpr int to_padded(int r, int c)
  {
    return (r + 2) * padded_board_side + c + 2;
  }

  static constexpr int to_padded(int loc)
  {
    return to_padded(loc / board_side, loc % board_side);
  }

  static constexpr int from_padded(int loc)
  {
    int padded_row = loc / padded_board_side;
    int padded_col = loc % padded_board_side;
    return (padded_row - 2) * board_side + padded_col - 2;
  }

  bool will_be_in_check(int, int) const;
  void add_pawn_moves(int, std::vector<Move>&) const;
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

  king_locations_ = { to_padded(0, 3), to_padded(5, 3) };
}

bool BoardImpl::will_be_in_check(int start, int end) const
{
  int in_check = false;

  int king_loc = king_locations_[static_cast<int>(player_to_move_)];
  int left_pawn_loc, right_pawn_loc;
  const Square start_sq = squares_[start];
  const auto pt = square::get_pt(start_sq);
  if (pt == PieceType::KING)
  {
    king_loc = end;
  }

  // Clear the start and fill the end.
  const Square end_sq = squares_[end];
  squares_[start] = 0x1; // Empty and not padding.
  squares_[end] = start_sq;

  // We've effectively made the move. Now check if the king's square is attacked.
  Square target_sq;
  for (auto target_pt : { PieceType::KNIGHT, PieceType::KING, PieceType::ROOK, PieceType::QUEEN})
  {
    for (const auto offset : piece_offsets[static_cast<int>(target_pt)])
    {
      if (offset == 0) break;
      int target_loc = king_loc + offset;
      target_sq = squares_[target_loc];
      while (square::on_board(target_sq))
      {
        auto pt = square::get_pt(target_sq);
        if (pt == target_pt &&
            square::get_colour(target_sq) != player_to_move_)
        {
          in_check = true;
          goto check_end;
        }
        if (pt != PieceType::NONE) break;
        if (target_pt == PieceType::KNIGHT || target_pt == PieceType::KING) break;
        target_loc += offset;
        target_sq = squares_[target_loc];
      }
    }
  }

  // Check for pawn checks.
  left_pawn_loc = king_loc - 1;
  left_pawn_loc += player_to_move_ == Colour::WHITE ?  padded_board_side : -padded_board_side;

  target_sq = squares_[left_pawn_loc];
  if (square::on_board(target_sq) &&
      square::get_pt(target_sq) == PieceType::PAWN &&
      square::get_colour(target_sq) != player_to_move_)
  {
    in_check = true;
    goto check_end;
  }

  right_pawn_loc = king_loc + 1;
  right_pawn_loc += player_to_move_ == Colour::WHITE ?  padded_board_side : -padded_board_side;

  target_sq = squares_[right_pawn_loc];
  if (square::on_board(target_sq) &&
      square::get_pt(target_sq) == PieceType::PAWN &&
      square::get_colour(target_sq) != player_to_move_)
  {
    in_check = true;
    goto check_end;
  }

check_end:
  // Revert.
  squares_[start] = start_sq;
  squares_[end] = end_sq;

  return in_check;
}

void BoardImpl::add_pawn_moves(int loc, std::vector<Move>& moves) const
{
  const int forward_offset =
    player_to_move_ == Colour::WHITE ?  padded_board_side : -padded_board_side;

  // Can we move forward to an empty location?
  const int forward = loc + forward_offset;
  Square target_sq = squares_[forward];
  if (square::get_pt(target_sq) == PieceType::NONE && !will_be_in_check(loc, forward))
  {
    moves.push_back(move::create(loc, forward));
  }

  // Can we capture diagonally?
  const int left_diag = forward - 1;
  target_sq = squares_[left_diag];

  if (target_sq & 0xFF)
  {
    auto pt = square::get_pt(target_sq);
    if (pt != PieceType::NONE &&
        square::get_colour(target_sq) != player_to_move_ &&
        !will_be_in_check(loc, left_diag))
    {
      moves.push_back(move::create(loc, left_diag, pt));
    }
  }

  const int right_diag = forward + 1;
  target_sq = squares_[right_diag];

  if (target_sq & 0xFF)
  {
    auto pt = square::get_pt(target_sq);
    if (pt != PieceType::NONE &&
        square::get_colour(target_sq) != player_to_move_ &&
        !will_be_in_check(loc, right_diag))
    {
      moves.push_back(move::create(loc, right_diag, pt));
    }
  }
}

std::vector<Move> BoardImpl::get_moves() const
{
  // TODO: Finish implementing pawn moves and checks etc.
  Square target_sq;
  std::vector<Move> moves;
  for (int loc = 0; loc < padded_board_area; loc++)
  {
    const Square sq = squares_[loc];
    if (!(sq & 0xFF)) continue; // Ignore padding locations.

    const auto pt = square::get_pt(sq);
    if (pt == PieceType::NONE) continue; // Ignore empty locations.

    const auto colour = square::get_colour(sq);
    if (colour != player_to_move_) continue; // Ignore pieces of the wrong colour.

    // Generate moves for this piece.
    const auto& offsets = piece_offsets[static_cast<int>(pt)];

    switch (pt)
    {
      case PieceType::PAWN:
        add_pawn_moves(loc, moves);
        break;
      case PieceType::KNIGHT: case PieceType::KING:
        for (const auto offset : offsets)
        {
          if (offset == 0) break;
          int target = loc + offset;
          target_sq = squares_[target];
          if (!square::on_board(target_sq)) continue;
          if (square::get_pt(target_sq) != PieceType::NONE &&
              square::get_colour(target_sq) == player_to_move_) continue;
          if (will_be_in_check(loc, target)) continue;
          moves.push_back(move::create(loc, target));
        }
        break;
      case PieceType::ROOK: case PieceType::QUEEN:
        for (const auto offset : offsets)
        {
          if (offset == 0) break;
          int target = loc + offset;
          target_sq = squares_[target];
          while (square::on_board(target_sq))
          {
            if (square::get_pt(target_sq) != PieceType::NONE)
            {
              if (square::get_colour(target_sq) != player_to_move_)
              {
                // Capture move.
                if (will_be_in_check(loc, target)) break;
                moves.push_back(move::create(loc, target));
              }

              break;
            }

            if (will_be_in_check(loc, target)) break;
            moves.push_back(move::create(loc, target));
            target += offset;
            target_sq = squares_[target];
          }
        }
        break;
    }
  }

  return moves;
}

std::vector<int> BoardImpl::get_targets_for_piece(int row, int col) const
{
  int loc = to_padded(row, col);

  // Get all moves and filter them based on start location.
  std::vector<int> targets;
  const auto moves = get_moves();

  for (const auto move : moves)
  {
    int start = move & 0xFF;
    if (start == loc)
    {
      int end = from_padded((move >> 8) & 0xFF);
      if (std::find(std::cbegin(targets), std::cend(targets), end) == std::cend(targets))
      {
        targets.push_back(end);
      }
    }
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

  const auto promo_type = move::get_promo(move);
  if (promo_type != PieceType::NONE)
  {
    end_sq |= static_cast<int>(promo_type) << 8;
  }
  else
  {
    end_sq |= moving_piece_type;
  }

  end_sq &= 0x00FFFF;
  end_sq |= static_cast<int>(player_to_move_) << 16;

  if ((moving_piece_type >> 8) == static_cast<int>(PieceType::KING))
  {
    king_locations_[static_cast<int>(player_to_move_)] = end;
  }

  player_to_move_ = player_to_move_ == Colour::WHITE ? Colour::BLACK : Colour::WHITE;
}

void BoardImpl::make_move(int start, int end, PieceType promo)
{
  const auto& sq = squares_[to_padded(end)];
  auto cap = static_cast<PieceType>((sq & 0xFF00) >> 8);
  Move move = move::create(to_padded(start), to_padded(end), cap, promo);
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

void Board::make_move(int start, int end, PieceType promo)
{
  impl_->make_move(start, end, promo);
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
