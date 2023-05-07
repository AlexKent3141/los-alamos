#include "search/search.h"

#include <cstdio>
#include <cstdlib>

int main()
{
  la::Board board;

  const auto callback = [&board] (const la::SearchData& data)
  {
    std::printf(
      "%6d %6s %7d %13ld %10ldms\n",
      data.depth,
      board.move_to_string(data.best_move).c_str(),
      data.score,
      data.nodes_searched,
      data.time_taken.count());

    std::fflush(stdout);
  };

  constexpr std::chrono::milliseconds search_time{60000};

  la::search(board, search_time, callback);

  return EXIT_SUCCESS;
}
