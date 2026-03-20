#include "wrapping_integers.hh"
#include "debug.hh"
#include <cmath>

using namespace std;

// n = absolute sequence number
// zero_point = initial sequence number
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  debug( "unimplemented wrap( {}, {} ) called", n, zero_point.raw_value_ );
  uint32_t zero = 0;
  uint64_t offset = n + zero_point.raw_value_;
  while(offset > ~zero){
    offset -= ~zero;
    offset--;
  }
  
  return Wrap32 { (uint32_t)offset };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  debug( "unimplemented unwrap( {}, {} ) called", zero_point.raw_value_, checkpoint );
  uint64_t max = (uint64_t)1 << 32;
  uint64_t n = checkpoint / max;

  if(n == 0){
    return checkpoint + 1;
  }

  uint64_t lower = (n - 1) * max + zero_point.raw_value_;
  uint64_t mid = n * max + zero_point.raw_value_;
  uint64_t upper = (n + 1) * max + zero_point.raw_value_;

  uint64_t l = (checkpoint > lower) ? checkpoint - lower: (lower - checkpoint);
  uint64_t m = (checkpoint > mid) ? checkpoint - mid: (mid - checkpoint);
  uint64_t u = (checkpoint > upper) ? checkpoint - upper: (upper - checkpoint);

  uint64_t cand = std::min(l, std::min(m, u));

  return cand * max + zero_point.raw_value_;


  // return {};
}
