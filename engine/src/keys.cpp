#include "keys.h"

namespace la
{

std::uint64_t keys::white_key = 0;
std::uint64_t keys::piece_square_keys[2][num_piece_types][padded_board_area] = { 0 };

keys::keys()
{
  struct PRNG
  {
    std::uint64_t x, y;
  };

  // Generate the hash keys.
  const auto xorshift64star = [] (std::uint64_t& x)
  {
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x * 0x2545F4914F6CDD1DULL;
  };

  // The top 32 bits are more random, so combine the top words from two instances.
  const auto prng_step = [xorshift64star] (PRNG& prng)
  {
    std::uint64_t word = (xorshift64star(prng.x) >> 32) << 32;
    word |= (xorshift64star(prng.y) >> 32);
    return word;
  };

  // Seed
  PRNG prng = { 0x6ADC22FF67CDB2AF, 0xDEADBEEF12345678 };

  white_key = prng_step(prng);

  for (int col = 0; col < 2; col++)
  {
    for (int pt = 0; pt < num_piece_types; pt++)
    {
      for (int loc = 0; loc < padded_board_area; loc++)
      {
        piece_square_keys[col][pt][loc] = prng_step(prng);
      }
    }
  }
}

}

namespace
{

static auto keys_instance = la::keys();

}
