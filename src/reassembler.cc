#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // if 
  if(first_index == currentIndex){
    output_.push(data);
    currentIndex += data.length();
  } else if (first_index < currentIndex && first_index + data.length() > currentIndex){
    output_.push(data.substr(first_index + data.length() - currentindex)); 
    currentIndex += first_index + data.length() - currentindex;
  } else {

    // add to set 
    for(int i = 0; i < data.length(); i++){
      s.insert({i + first_index, data[i]});
    }
  }

  // post check to remove any no longer valid data 
  // AND check if part of the index is now available again

  // Close the stream
  if(is_last_substring){
    output_.close();
  }
  return;
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  return s.size();
}
