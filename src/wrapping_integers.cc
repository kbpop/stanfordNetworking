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
  // uint64_t offset = UINT32_MAX;
  // while (checkpoint > offset)
  // {
  //   offset = offset << 1;
  //   offset += UINT32_MAX;
  // }

  // return offset + (uint64_t)zero_point.raw_value_;
  return {};
}
