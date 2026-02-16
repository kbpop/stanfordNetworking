#include "reassembler.hh"
#include "debug.hh"
#include <iostream>

using namespace std;

bool Reassembler::_check_left_overlap( uint64_t &new_start, uint64_t &new_end, std::string &new_data ){
  auto it = m.lower_bound(new_start);
  if(it != m.begin()){
    auto prev_it = std::prev(it);
    uint64_t prev_start = prev_it->first; 
    uint64_t prev_end = prev_it->first + prev_it->second.length(); 
    
    if(prev_end >= new_start){
      if(prev_end >= new_end){
        // swallows the entire string
        return true;
      } else {
        new_data = prev_it->second + new_data.substr(prev_end - new_start);
        new_start = prev_start; 
        m.erase(prev_it);
      }
    }
  }
  return false;
}

void Reassembler::_check_right_overlap( uint64_t &new_start, uint64_t &new_end, std::string &new_data )
{
  auto it = m.lower_bound(new_start);

  while(it != m.end() && it->first < new_end){
    uint64_t next_start = it->first;
    uint64_t next_end = next_start + it->second.length();

    if(next_end > new_end){
      new_data += it->second.substr(new_end - next_start);
      new_end = next_end;
    }
    it = m.erase(it);
  }
}

void Reassembler::_store( uint64_t first_index, string &data)
{
  if(data.empty()){
    return;
  }

  uint64_t new_start = first_index; 
  uint64_t new_end = first_index + data.length(); 
  std::string new_data = data;
  

  if(_check_left_overlap(new_start, new_end, new_data)) return;
  _check_right_overlap(new_start, new_end, new_data);  
  m[new_start] = new_data;
}

void Reassembler::add_stream(std::string data)
{
    size_t end = std::min(output_.writer().available_capacity(), data.length());
    output_.writer().push(data.substr(0, end));
    current_index += end;
}

void Reassembler::check_store()
{
    while(m.find(current_index) != m.end() && output_.writer().available_capacity() > 0){
      uint64_t removal = current_index;
      // push entire data
      if(output_.writer().available_capacity() >= m[current_index].length()){
        add_stream(m[current_index]);
      } else {
      // Split data to appropriate stream
        m[current_index + output_.writer().available_capacity()] = m[current_index].substr(output_.writer().available_capacity());
        add_stream(m[current_index].substr(0, output_.writer().available_capacity()));
      }
      m.erase(removal);
    }
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring ){
  uint64_t max_index = current_index + output_.writer().available_capacity();

  if(is_last_substring){
    eof_arrived = true;
    eof_index = first_index + data.length();
  }

  // Filter initial data
  if(!(data.empty() || first_index >= max_index || first_index + data.length() <= current_index)){

  // truncate string for corresponding capacity 
    if(first_index + data.length() > max_index){
      data = data.substr(0, max_index - first_index);
    } 

    // push the string up to where current index is
    if(first_index < current_index){
      data = data.substr(current_index - first_index);
      first_index = current_index;
    }

    // Add to store
    if(!data.empty()){
      _store(first_index, data);
    }
  }

  check_store();

  if(eof_arrived && current_index >= eof_index){
    output_.writer().close();
  }
  count_bytes_pending();
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t count = 0;
  for (const auto& pair: m){
    std::cerr << "first: " << pair.first << ", string: " << pair.second << std::endl;
    count += pair.second.length();
  }
  return count;
}
