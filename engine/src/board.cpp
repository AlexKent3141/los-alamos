#include "engine/engine.h"

namespace la
{

class BoardImpl
{
public:
  BoardImpl();
  std::vector<Move> get_moves() const;
  void make_move(Move);
  void undo_move(Move);

private:
};

BoardImpl::BoardImpl()
{
  // TODO: Initialise default board.
}

std::vector<Move> BoardImpl::get_moves() const
{
  // TODO
  return std::vector<Move>();
}

void BoardImpl::make_move(Move move)
{
  // TODO
}

void BoardImpl::undo_move(Move move)
{
  // TODO
}

Board::Board() : impl_(std::make_unique<BoardImpl>())
{
}

std::vector<Move> Board::get_moves() const
{
  return impl_->get_moves();
}

void Board::make_move(Move move)
{
  impl_->make_move(move);
}

void Board::undo_move(Move move)
{
  impl_->undo_move(move);
}

}
