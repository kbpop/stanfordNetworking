#include "wrapping_integers.hh"
#include "debug.hh"
#include <cmath>

using namespace std;

// n = absolute sequence number
// zero_point = initial sequence number
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { (uint32_t)n + zero_point.raw_value_ };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint64_t max = 1ULL << 32;
  uint64_t offset = this->raw_value_ - zero_point.raw_value_;
  uint64_t n = checkpoint / max;

  uint64_t mid = n * max + offset;
  uint64_t lower = (n > 0) ? (n - 1) * max + offset : mid;
  uint64_t upper = (n + 1) * max + offset;

  uint64_t l = (checkpoint > lower) ? checkpoint - lower: (lower - checkpoint);
  uint64_t m = (checkpoint > mid) ? checkpoint - mid: (mid - checkpoint);
  uint64_t u = (checkpoint > upper) ? checkpoint - upper: (upper - checkpoint);

  return (l <= m && l <= u) ? lower : 
            (m <= u) ? mid 
            : upper;


  // return {};
}
