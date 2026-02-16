#include "reassembler.hh"
#include "debug.hh"
#include <iostream>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{

  // Unacceptable filter
  if(first_index + data.length() > output_.writer().available_capacity() ){
    return;
  }

  // push to stream right away
  if(first_index == current_index || (first_index < current_index && first_index + data.length() >= current_index)){

    if(first_index != current_index){
      data = data.substr(current_index - first_index);
    }
    
    add_stream(data);
    check_store();

  } else if (first_index > current_index){
    // store data for later access

    // Add to map
    auto it = m.lower_bound(first_index);
    uint64_t begin = 0; 
  
    if (it != m.end() && it->first < first_index + data.length()) { 
      first_index = it->first + it->second.length();
      begin = first_index - it->first;
    } 

    m[first_index] = data.substr(begin);
    add_store(data, first_index);

  } else {
    return;
  }

  if(is_last_substring){
    output_.writer().close();
  }

}

void Reassembler::add_stream(std::string data){
    int end = std::min(output_.writer().available_capacity(), data.length());
    output_.writer().push(data.substr(0, end));
    current_index += end;
}

void Reassembler::add_store(std::string data, uint64_t index){
    int end = std::min(output_.writer().available_capacity(), data.length());
    m[index] = data.substr(0, end);
}

void Reassembler::check_store(){
    while(m.find(current_index) != m.end() && output_.writer().available_capacity() > 0){
      if(output_.writer().available_capacity() > m[current_index].length()){
        add_stream(m[current_index]);
      } else {
        m[current_index + output_.writer().available_capacity()] = m[current_index].substr(output_.writer().available_capacity());
        add_stream(m[current_index].substr(0, output_.writer().available_capacity()));
      }
      m.erase(current_index);
    }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  return m.size();
}
