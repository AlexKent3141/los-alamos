#include "engine/engine.h"

#include "eval.h"
#include "keys.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <stack>

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

inline bool on_board(Square sq)
{ return sq & 0xFF; }
inline la::PieceType get_pt(Square sq)
{ return static_cast<la::PieceType>((sq & 0xFF00) >> 8); }
inline void set_pt(Square& sq, la::PieceType pt)
{ sq |= (static_cast<int>(pt) << 8); }
inline la::Colour get_colour(Square sq)
{ return static_cast<la::Colour>((sq & 0xFF0000) >> 16); }
inline void set_colour(Square& sq, la::Colour col)
{ sq |= (static_cast<int>(col) << 16); }

inline bool is_pawn(Square sq)
{
  const la::PieceType pt = get_pt(sq);
  return pt == la::PieceType::PAWN_WHITE || pt == la::PieceType::PAWN_BLACK;
}

}

namespace move
{

inline int get_start(la::Move m)
{ return m & 0xFF; }
inline int get_end(la::Move m)
{ return (m & 0xFF00) >> 8; }
inline la::PieceType get_cap(la::Move m)
{ return static_cast<la::PieceType>((m & 0xFF0000) >> 16); }
inline la::PieceType get_promo(la::Move m)
{ return static_cast<la::PieceType>((m & 0xFF000000) >> 24); }
inline void set_promo(la::Move& m, la::PieceType pt)
{ m |= static_cast<int>(pt) << 24; }

inline la::Move create(
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
  int score() const { return scores_.top(); }
  bool in_check() const;
  std::optional<Piece> get_piece(int, int) const;
  std::string move_to_string(Move) const;

private:
  // Padded offsets for each piece's moves.
  static constexpr std::array<
    std::array<int, 8>,
    static_cast<int>(PieceType::NUM_PIECE_TYPES)> piece_offsets =
  {
    std::array<int, 8>
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    std::array<int, 8>
    { padded_board_side, 0, 0, 0, 0, 0, 0, 0 },
    std::array<int, 8>
    { -padded_board_side, 0, 0, 0, 0, 0, 0, 0 },
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
  std::stack<int> scores_;

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

  int score = 0;

  const auto set_square_properties = [&] (int loc, Colour col, PieceType pt)
  {
    Square& sq = squares_[loc];
    square::set_pt(sq, pt);
    square::set_colour(sq, col);

    const int piece_score =
      eval::piece_scores[static_cast<int>(pt)] +
      eval::square_scores[static_cast<int>(pt)][loc];

    score += col == Colour::WHITE ? piece_score : -piece_score;
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
    set_square_properties(to_padded(0, c), Colour::WHITE, backrank[c]);
    set_square_properties(to_padded(1, c), Colour::WHITE, PieceType::PAWN_WHITE);
    set_square_properties(to_padded(4, c), Colour::BLACK, PieceType::PAWN_BLACK);
    set_square_properties(to_padded(5, c), Colour::BLACK, backrank[c]);
  }

  king_locations_ = { to_padded(0, 3), to_padded(5, 3) };

  scores_.push(score);
}

bool BoardImpl::will_be_in_check(int start, int end) const
{
  bool in_check = false;

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
      square::is_pawn(target_sq) &&
      square::get_colour(target_sq) != player_to_move_)
  {
    in_check = true;
    goto check_end;
  }

  right_pawn_loc = king_loc + 1;
  right_pawn_loc += player_to_move_ == Colour::WHITE ?  padded_board_side : -padded_board_side;

  target_sq = squares_[right_pawn_loc];
  if (square::on_board(target_sq) &&
      square::is_pawn(target_sq) &&
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
  const auto add_promotions = [&moves] (const Move& move)
  {
    Move knight_promo(move);
    move::set_promo(knight_promo, PieceType::KNIGHT);
    moves.push_back(knight_promo);
    Move rook_promo(move);
    move::set_promo(rook_promo, PieceType::ROOK);
    moves.push_back(rook_promo);
    Move queen_promo(move);
    move::set_promo(queen_promo, PieceType::QUEEN);
    moves.push_back(queen_promo);
  };

  const int forward_offset =
    player_to_move_ == Colour::WHITE ?  padded_board_side : -padded_board_side;

  // Can we move forward to an empty location?
  const int forward = loc + forward_offset;
  Square target_sq = squares_[forward];
  if (square::get_pt(target_sq) == PieceType::NONE && !will_be_in_check(loc, forward))
  {
    const auto move = move::create(loc, forward);
    if (forward < 3 * padded_board_side || forward >= 7 * padded_board_side)
    {
      add_promotions(move);
    }
    else
    {
      moves.push_back(move);
    }
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
      const auto move = move::create(loc, left_diag, pt);
      if (left_diag < 3 * padded_board_side || left_diag >= 7 * padded_board_side)
      {
        add_promotions(move);
      }
      else
      {
        moves.push_back(move);
      }
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
      const auto move = move::create(loc, right_diag, pt);
      if (right_diag < 3 * padded_board_side || right_diag >= 7 * padded_board_side)
      {
        add_promotions(move);
      }
      else
      {
        moves.push_back(move);
      }
    }
  }
}

std::vector<Move> BoardImpl::get_moves() const
{
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

    if (pt == PieceType::PAWN_WHITE || pt == PieceType::PAWN_BLACK)
    {
      add_pawn_moves(loc, moves);
    }
    else
    {
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
              moves.push_back(move::create(loc, target, square::get_pt(target_sq)));
            }

            break;
          }

          if (!will_be_in_check(loc, target))
          {
            moves.push_back(move::create(loc, target));
          }

          if (pt == PieceType::KNIGHT || pt == PieceType::KING) break;

          target += offset;
          target_sq = squares_[target];
        }
      }
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
  int next_score = scores_.top();

  const int start = move::get_start(move);
  const int end = move::get_end(move);
  const auto cap_piece_type = move::get_cap(move);

  auto& start_sq = squares_[start];
  auto& end_sq = squares_[end];

  const auto moving_piece_type = square::get_pt(start_sq);
  start_sq &= 0xFF;
  end_sq &= 0xFF;

  next_score -= eval::square_scores[static_cast<int>(moving_piece_type)][start];

  const auto promo_type = move::get_promo(move);
  if (promo_type != PieceType::NONE)
  {
    next_score -= eval::piece_scores[static_cast<int>(moving_piece_type)];
    next_score += eval::piece_scores[static_cast<int>(promo_type)];

    next_score += eval::square_scores[static_cast<int>(promo_type)][end];
    square::set_pt(end_sq, promo_type);
  }
  else
  {
    next_score += eval::square_scores[static_cast<int>(moving_piece_type)][end];
    square::set_pt(end_sq, moving_piece_type);
  }

  square::set_colour(end_sq, player_to_move_);

  next_score += eval::piece_scores[static_cast<int>(cap_piece_type)];

  if (moving_piece_type == PieceType::KING)
  {
    king_locations_[static_cast<int>(player_to_move_)] = end;
  }

  player_to_move_ = player_to_move_ == Colour::WHITE ? Colour::BLACK : Colour::WHITE;

  scores_.push(-1 * next_score);
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
  const auto other_player = player_to_move_;
  player_to_move_ = player_to_move_ == Colour::WHITE ? Colour::BLACK : Colour::WHITE;

  const int start = move::get_start(move);
  const int end = move::get_end(move);

  auto& start_sq = squares_[start];
  auto& end_sq = squares_[end];

  const auto moving_piece_type = square::get_pt(end_sq);
  start_sq &= 0xFF;
  end_sq &= 0xFF;

  const auto promo_type = move::get_promo(move);
  if (promo_type != PieceType::NONE)
  {
    if (other_player == Colour::WHITE)
    {
      square::set_pt(start_sq, PieceType::PAWN_WHITE);
    }
    else
    {
      square::set_pt(start_sq, PieceType::PAWN_BLACK);
    }
  }
  else
  {
    square::set_pt(start_sq, moving_piece_type);
  }

  // Set the colour.
  square::set_colour(start_sq, player_to_move_);

  const auto cap = move::get_cap(move);
  if (cap != PieceType::NONE)
  {
    square::set_pt(end_sq, cap);
    square::set_colour(end_sq, other_player);
  }

  if (moving_piece_type == PieceType::KING)
  {
    king_locations_[static_cast<int>(player_to_move_)] = start;
  }

  scores_.pop();
}

bool BoardImpl::in_check() const
{
  // Call `will_be_in_check` with a dummy move.
  int king_loc = king_locations_[static_cast<int>(player_to_move_)];
  return will_be_in_check(king_loc, king_loc);
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

// Serialise a move given the current position.
std::string BoardImpl::move_to_string(Move move) const
{
  static const auto promo_pt_to_str = [] (PieceType pt) -> std::string
  {
    switch (pt)
    {
      case PieceType::ROOK:   return "R"; break;
      case PieceType::KNIGHT: return "N"; break;
      case PieceType::QUEEN:  return "Q"; break;
      default:
        std::abort();
    }
  };

  static const auto loc_to_str = [] (int loc) -> std::string
  {
    static constexpr const char* cols = "abcdef";
    const int row = loc / padded_board_side - 1;
    const int col = loc % padded_board_side - 2;
    return std::string(1, cols[col]) + std::to_string(row);
  };

  const int start = move::get_start(move);
  const int end = move::get_end(move);

  std::string move_str = loc_to_str(start) + loc_to_str(end);

  // Append promotion string if needed.
  const auto promo = move::get_promo(move);
  if (promo != PieceType::NONE)
  {
    move_str += "=" + promo_pt_to_str(promo);
  }

  return move_str;
}

Board::Board() : impl_(std::make_unique<BoardImpl>())
{
}

Board::Board(const Board& other) : impl_(std::make_unique<BoardImpl>(*other.impl_))
{
}

Board& Board::operator=(const Board& other)
{
  impl_ = std::make_unique<BoardImpl>(*other.impl_);
  return *this;
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

int Board::score() const
{
  return impl_->score();
}

bool Board::in_check() const
{
  return impl_->in_check();
}

std::optional<Piece> Board::get_piece(int row, int col) const
{
  return impl_->get_piece(row, col);
}

std::string Board::move_to_string(Move move) const
{
  return impl_->move_to_string(move);
}

}
