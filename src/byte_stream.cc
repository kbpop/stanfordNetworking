#include "byte_stream.hh"
#include "debug.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buffer() {
}


// Push data to stream, but only as much as available capacity allows.
void Writer::push( string data )
{
  int m;
  if(size(data) + buffer.size() > capacity_ ){
    m = capacity_ - buffer.size();
  } else {
    m = size(data);
  }
  buffer.append(data.substr(0, m));
  bytes_pu += m;
  return;
  
}

// Signal that the stream has reached its ending. Nothing more will be written.
void Writer::close()
{
  closed = true;
  return;
}

// Has the stream been closed?
bool Writer::is_closed() const
{
  return closed;
}

// How many bytes can be pushed to the stream right now?
uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer.size();
}

// Total number of bytes cumulatively pushed to the stream
uint64_t Writer::bytes_pushed() const
{
  return bytes_pu;
}

// Peek at the next bytes in the buffer -- ideally as many as possible.
// It's not required to return a string_view of the *whole* buffer, but
// if the peeked string_view is only one byte at a time, it will probably force
// the caller to do a lot of extra work.
string_view Reader::peek() const
{
  string_view s = buffer;
  return s;
}

// Remove `len` bytes from the buffer.
void Reader::pop( uint64_t len )
{
  buffer.erase(0,len);
  bytes_po += len;
  return;
}

// Is the stream finished (closed and fully popped)?
bool Reader::is_finished() const
{
  return closed && buffer.empty();
}

// Number of bytes currently buffered (pushed and not popped)
uint64_t Reader::bytes_buffered() const
{
  return buffer.size();
}

// Total number of bytes cumulatively popped from stream
uint64_t Reader::bytes_popped() const
{
  return bytes_po;
}
