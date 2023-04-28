#pragma once

#include <array>

// This namespace contains compile-time constants we use to evaluate the position.
namespace la::eval
{

// Scores for each type of material.
constexpr std::array<int, 7> piece_scores = { 0, 100, 100, 300, 500, 900, 0 };

// For each type of material assign a bonus for that material at each location on the board.
// This will hopefully give the program an understanding of the need to
// * centralise pieces 
// * advance pawns
// * ...
// We're using padded coordinates to save some converting later, so there are two lines of
// empty padding all the way around.
constexpr std::array<std::array<int, 100>, 7> square_scores =
{
  /* Padding for the NONE piece type. */
  std::array<int, 100> {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0
  },
  /* PAWN_WHITE
     These scores are from the first player's perspective. */
  std::array<int, 100> {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  2,  2,  2,  2,  2,  2,  0,  0,
     0,  0,  5,  5,  7,  7,  5,  5,  0,  0,
     0,  0, 10, 10, 10, 10, 10, 10,  0,  0,
     0,  0, 30, 30, 30, 30, 30, 30,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0
  },
  /* PAWN_BLACK
     These scores are from the first player's perspective. */
  std::array<int, 100> {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0, 30, 30, 30, 30, 30, 30,  0,  0,
     0,  0, 10, 10, 10, 10, 10, 10,  0,  0,
     0,  0,  5,  5,  7,  7,  5,  5,  0,  0,
     0,  0,  2,  2,  2,  2,  2,  2,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0
  },
  /* KNIGHT
     Knights on the rim are dim. */
  std::array<int, 100> {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0, -5, -5, -5, -5, -5, -5,  0,  0,
     0,  0, -5,  5,  5,  5,  5, -5,  0,  0,
     0,  0, -5,  5, 10, 10,  5, -5,  0,  0,
     0,  0, -5,  5, 10, 10,  5, -5,  0,  0,
     0,  0, -5,  5,  5,  5,  5, -5,  0,  0,
     0,  0, -5, -5, -5, -5, -5, -5,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0
  },
  /* ROOK 
     Not sure rooks gain much positional advantage anywhere, but slightly weight the centre. */
  std::array<int, 100> {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  1,  1,  1,  1,  0,  0,  0,
     0,  0,  0,  1,  1,  1,  1,  0,  0,  0,
     0,  0,  0,  1,  1,  1,  1,  0,  0,  0,
     0,  0,  0,  1,  1,  1,  1,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0
  },
  /* QUEEN
     The queen is probably a bit better in the centre. */
  std::array<int, 100> {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  2,  2,  2,  2,  0,  0,  0,
     0,  0,  0,  2,  5,  5,  2,  0,  0,  0,
     0,  0,  0,  2,  5,  5,  2,  0,  0,  0,
     0,  0,  0,  2,  2,  2,  2,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0
  },
  /* KING
     The king is probably a bit better in the centre. */
  std::array<int, 100> {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  2,  2,  2,  2,  0,  0,  0,
     0,  0,  0,  2,  5,  5,  2,  0,  0,  0,
     0,  0,  0,  2,  5,  5,  2,  0,  0,  0,
     0,  0,  0,  2,  2,  2,  2,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0
  }
};

}
