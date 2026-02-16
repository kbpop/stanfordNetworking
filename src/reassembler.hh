#pragma once

#include <string>
#include <map>
#include "byte_stream.hh"

class Reassembler
{

private:
  void _insert( uint64_t first_index, std::string data, bool is_last_substring );


public:
  // Construct Reassembler to write into given ByteStream.
  
  uint64_t size;
  uint64_t current_index;
  std::map<uint64_t, std::string> m;
  bool eof_arrived;
  bool eof_index;

  explicit Reassembler( ByteStream&& output ) : size(0), current_index(0), m{},eof_arrived(false), eof_index(0), output_( std::move( output ) ) {}

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  void add_stream( std::string data);
  void add_store( std::string data, uint64_t index);
  void check_store();

  // How many bytes are stored in the Reassembler itself?
  // This function is for testing only; don't add extra state to support it.
  uint64_t count_bytes_pending() const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }

private:
  ByteStream output_;
};
