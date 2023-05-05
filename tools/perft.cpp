#include "engine/board.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>

#if defined(PERFT_USE_TABLE)
#include "engine/tt.h"

struct Entry
{
  std::uint64_t hash;
  int depth;
  std::size_t num_child_nodes;
};

using Table = la::TT<Entry, 65536>;
#else
struct Table {};
#endif

std::uint64_t perft(la::Board& board, int depth, Table& tt)
{
  if (depth == 0) return 1;

#if defined(PERFT_USE_TABLE)
  Entry* entry;
  if (tt.probe(board.hash(), &entry) && depth > 2 && entry->depth == depth)
  {
    return entry->num_child_nodes;
  }
#endif

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
    total += perft(board, depth - 1, tt);
    board.undo_move(move);
  }

#if defined(PERFT_USE_TABLE)
  if (depth > entry->depth)
  {
    entry->hash = board.hash();
    entry->depth = depth;
    entry->num_child_nodes = total;
  }
#endif

  return total;
}

int main()
{
  std::cout << "Calculating perft\n";

  Table tt;

  using Clock = std::chrono::steady_clock;
  const auto start = Clock::now();
  la::Board board;
  for (int d = 1; d < 9; d++)
  {
    auto perft_val = perft(board, d, tt);

    const auto end = Clock::now();
    const auto secs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Depth: " << std::setw(5) << d
              << ", Perft: " << std::setw(15) << perft_val
              << ", Time taken: " << std::setw(10) << secs.count() << "ms"
              << "\n";
  }

  return EXIT_SUCCESS;
}
