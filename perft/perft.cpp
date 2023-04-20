#include "engine/engine.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>

std::uint64_t perft(la::Board& board, int depth)
{
  if (depth == 0) return 1;

  const auto moves = board.get_moves();
  if (depth == 1)
  {
    return moves.size();
  }

  std::uint64_t total = 0;
  for (std::size_t i = 0; i < moves.size(); i++)
  {
    const la::Move move = moves[i];
    board.make_move(move);
    total += perft(board, depth - 1);
    board.undo_move(move);
  }

  return total;
}

int main()
{
  std::cout << "Calculating perft\n";

  using Clock = std::chrono::steady_clock;
  const auto start = Clock::now();
  la::Board board;
  for (int d = 1; d < 9; d++)
  {
    auto perft_val = perft(board, d);

    const auto end = Clock::now();
    const auto secs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Depth: " << std::setw(5) << d
              << ", Perft: " << std::setw(15) << perft_val
              << ", Time taken: " << std::setw(10) << secs.count() << "ms"
              << "\n";
  }

  return EXIT_SUCCESS;
}
