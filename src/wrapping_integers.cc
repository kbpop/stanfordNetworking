#include "wrapping_integers.hh"
#include "debug.hh"

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
  uint64_t n = checkpoint / UINT32_MAX;
  uint64_t lower = (n - 1) * UINT32_MAX + zero_point.raw_value_;

  if(n == 0){
    return UINT32_MAX + zero_point.raw_value_;
  }

  for(int i = 0; i < 3; i++){
    if(checkpoint > lower || checkpoint < (lower + (UINT32_MAX >> 1)) ){
      break;
    }

    lower += UINT32_MAX;
  }

  return lower;

  // return {};
}
