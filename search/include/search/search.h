#pragma once

#include "engine/board.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

namespace la
{

struct SearchData
{
  int depth;
  int score;
  la::Move best_move;
  std::uint64_t nodes_searched;
};

// Blocking search.
Move search(la::Board&, std::chrono::milliseconds, std::function<void(const SearchData&)>);

class SearchWorker
{
public:
  SearchWorker(std::function<void(const SearchData&)>);
  ~SearchWorker();

  void start(const la::Board&, std::chrono::milliseconds);
  bool running() const { return running_.load(); }

private:
  std::function<void(const SearchData&)> callback_;
  std::atomic<bool> running_;
  std::thread worker_;
  Board board_;
  std::chrono::milliseconds timeout_;

  void search_worker_func();
};

}
