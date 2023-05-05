#include "search/search.h"

#include <cassert>

namespace
{

using Clock = std::chrono::steady_clock;

Clock::time_point current_search_end_time;
std::uint64_t num_nodes_searched;

using Table = la::TT<Entry, 2000000>;

bool in_time()
{
  return Clock::now() < current_search_end_time;
}

int minimax(la::Board& board, int depth)
{
  if (depth == 0)
  {
    ++num_nodes_searched;
    return board.score();
  }

  const auto moves = board.get_moves();
  if (moves.empty())
  {
    // The game is over and we're either in stalemate or checkmate.
    if (board.in_check())
    {
      return -la::eval::mate_score;
    }
    else
    {
      return 0;
    }
  }

  int best_score = -la::eval::mate_score, score;
  for (const auto move : moves)
  {
    // Return early if we're out of time.
    if (!in_time())
    {
      return 0;
    }

    board.make_move(move);
    score = -minimax(board, depth - 1);
    board.undo_move(move);

    if (score > best_score)
    {
      best_score = score;
    }
  }

  return best_score;
}

}

namespace la
{

Move search(
  la::Board& board,
  std::chrono::milliseconds timeout,
  std::function<void(const SearchData&)> callback)
{
  current_search_end_time = Clock::now() + timeout;

  const auto moves = board.get_moves();

  assert(!moves.empty());

  int depth = 1, score, best_score, best_score_at_depth;
  Move best_move = moves[0], best_move_at_depth = moves[0];
  while (in_time())
  {
    num_nodes_searched = 0;

    // Try all available moves and keep track of the one with the highest score.
    best_score_at_depth = -eval::mate_score;
    for (auto move : moves)
    {
      if (!in_time()) break;

      board.make_move(move);
      score = -minimax(board, depth - 1);
      board.undo_move(move);

      if (score > best_score_at_depth)
      {
        best_score_at_depth = score;
        best_move_at_depth = move;
      }
    }

    // If we're still in time then we can update our best data.
    if (in_time())
    {
      best_move = best_move_at_depth;
      best_score = best_score_at_depth;

      SearchData data = { depth, best_score, best_move, num_nodes_searched };
      callback(data);
    }

    ++depth;
  }

  return best_move;
}

SearchWorker::SearchWorker(std::function<void(const SearchData&)> callback)
  : callback_(callback), running_(false)
{
}

SearchWorker::~SearchWorker()
{
  if (worker_.joinable())
  {
    worker_.join();
  }
}

void SearchWorker::start(const la::Board& board, std::chrono::milliseconds timeout)
{
  running_.store(true);

  board_ = board;
  timeout_ = timeout;

  if (worker_.joinable())
  {
    worker_.join();
  }

  worker_ = std::thread(&SearchWorker::search_worker_func, this);
}

void SearchWorker::search_worker_func()
{
  search(board_, timeout_, callback_);
  running_.store(false);
}

}
