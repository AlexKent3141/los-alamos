#include "search/search.h"
#include "engine/tt.h"

#include <cassert>
#include <utility>

namespace
{

using Clock = std::chrono::steady_clock;

Clock::time_point current_search_end_time;
std::uint64_t num_nodes_searched;

struct Entry
{
  std::uint64_t hash;
  int depth;
  int score;
  la::Move hash_move;
};

using Table = la::TT<Entry, 2000000>;

bool in_time()
{
  return Clock::now() < current_search_end_time;
}

// Search only the dynamic moves to try and get to a quiet position.
// Playing a move in this stage is optional, so we need to keep track of a `stand-pat` value.
int quiesce(la::Board& board, int depth, int alpha, int beta)
{
  if (depth == 0)
  {
    return board.score();
  }

  const int stand_pat = board.score();
  if (stand_pat >= beta)
  {
    return beta;
  }

  if (alpha < stand_pat)
  {
    alpha = stand_pat;
  }

  const auto moves = board.get_moves(la::MoveGenType::DYNAMIC);
  int score;
  for (const auto move : moves)
  {
    board.make_move(move);
    score = -quiesce(board, depth - 1, -beta, -alpha);
    board.undo_move(move);

    alpha = std::max(alpha, score);
    if (alpha >= beta)
    {
      return beta;
    }
  }

  return alpha;
}

int minimax(la::Board& board, int depth, int alpha, int beta, Table& table)
{
  if (depth == 0)
  {
    ++num_nodes_searched;
    return quiesce(board, 3, alpha, beta);
  }

  la::Move hash_move = 0;
  Entry* entry;
  if (table.probe(board.hash(), &entry))
  {
    if (entry->depth >= depth)
    {
      alpha = std::max(alpha, entry->score);
    }

    hash_move = entry->hash_move;
  }

  auto moves = board.get_moves();
  if (moves.empty())
  {
    if (board.is_draw())
    {
      // Draw by repetition.
      return 0;
    }
    else if (board.in_check())
    {
      // Checkmate.
      return -la::eval::mate_score;
    }
    else
    {
      // Stalemate.
      return 0;
    }
  }

  // Sort moves so that captures are first.
  std::size_t cap_index = 0;
  for (std::size_t i = 0; i < moves.size(); i++)
  {
    if (la::move::get_cap(moves[i]) != la::PieceType::NONE)
    {
      std::swap(moves[cap_index++], moves[i]);
    }
  }

  // If we have a hash move then put it first.
  if (hash_move != 0)
  {
    for (std::size_t i = 0; i < moves.size(); i++)
    {
      if (moves[i] == hash_move)
      {
        std::swap(moves[i], moves[0]);
        break;
      }
    }
  }

  int best_score = -la::eval::mate_score, score;
  la::Move cutoff_move = 0;
  for (const auto move : moves)
  {
    // Return early if we're out of time.
    if (!in_time())
    {
      return 0;
    }

    board.make_move(move);
    score = -minimax(board, depth - 1, -beta, -alpha, table);
    board.undo_move(move);

    if (score > best_score)
    {
      best_score = score;
    }

    alpha = std::max(alpha, best_score);
    if (alpha >= beta)
    {
      // Cut-off
      cutoff_move = move;
      break;
    }
  }

  if (depth > entry->depth)
  {
    entry->hash = board.hash();
    entry->depth = depth;
    entry->score = alpha;
    entry->hash_move = cutoff_move;
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
  const auto start_time = Clock::now();
  current_search_end_time = start_time + timeout;

  const auto moves = board.get_moves();

  assert(!moves.empty());

  Table table;
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
      score = -minimax(board, depth - 1, -la::eval::mate_score, la::eval::mate_score, table);
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

      const auto time_taken =
        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start_time);

      SearchData data = { depth, best_score, best_move, num_nodes_searched, time_taken };
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
