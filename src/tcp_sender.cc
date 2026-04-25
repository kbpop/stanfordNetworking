#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// How many sequence numbers are outstanding?
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return outstanding_seqnos_;
}

// How many consecutive retransmissions have happened?
uint64_t TCPSender::consecutive_retransmissions() const
{
  return retransmission_number;
}

/* Turn bytestream input into something 
that can be sent from the sender*/
TCPSenderMessage TCPSender::StreamToMsg(){
  // set SYN
  bool SYN = last_n == 0;
 
  uint64_t min_win_size = (uint64_t)last_window_size;
  if(min_win_size >= (SYN)){
    min_win_size -= (SYN);
  }

  uint minSize = std::min(std::min(TCPConfig::MAX_PAYLOAD_SIZE, reader().bytes_buffered()), min_win_size);
  
  // std::string payload {};
  // make slice of payload
  // remove from queue
  std::string payload = "";
  while(payload.size() < minSize){
    std::string_view view = reader().peek();
    if(view.empty()){
      break;
    }
    std::string_view chunk = view.substr(0, minSize - payload.size());
    payload += chunk;
    reader().pop(chunk.size());
  }

  bool FIN = false;
  if(reader().is_finished() &&  reader().bytes_buffered() == 0){
    if(min_win_size - minSize >= 1){
      FIN = true;
    }
  }

 // Wrap32 seqno { 0 };
  Wrap32 seqno = Wrap32::wrap(last_n, isn_); 
  // bool RST {};
  bool RST = input_.has_error();
 
  // size_t sequence_length() const { return SYN + payload.size() + FIN; }
  TCPSenderMessage toSend = {
    .seqno = seqno,
    .SYN = SYN,
    .payload = payload,
    .FIN = FIN,
    .RST = RST,
  };
  last_n += toSend.sequence_length();
  last_window_size -= toSend.sequence_length();
  outstanding_seqnos_ += toSend.sequence_length();
  return toSend;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // if window size is zero send only one message
  // make sure TCPSenderMessage fits in receiver window
  // use TCPSenderMessage::sequence_length()
  if(last_window_size == 0 && sequence_numbers_in_flight() == 0){
    last_window_size = 1;
  }

  // 1. Check if it fits into the receiver window
  while(true){
    TCPSenderMessage payload = StreamToMsg();

    if(payload.sequence_length() == 0){
      break;
    }
    // 3. Add it to the queue
    flightMsg.q.push(payload);
      // 4. Actually send it 
    transmit(payload);
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  Wrap32 seqno = Wrap32::wrap(last_n, isn_); 
  bool RST = input_.has_error();
  TCPSenderMessage toSend = {
    .seqno = seqno,
    .SYN = false,
    .payload = "",
    .FIN = false,
    .RST = RST,
  };
  return toSend;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // shape to receive
  // std::optional<Wrap32> ackno {};
  // bool RST {};
  if(msg.RST){
    input_.set_error(); 
    return;
  } 

  last_window_size = msg.window_size;  

  if(!msg.ackno.has_value()){
    return;
  }
  // 1. remove outstanding segments of ackno + windowSize
  // 2. while loop of queue until a larger than ackno + windowsize is encountered
  uint64_t temp = msg.ackno.value().unwrap(isn_, last_n);
  while(!flightMsg.q.empty() 
        && temp <= last_n 
        && flightMsg.q.front().seqno.unwrap(isn_, last_n) + flightMsg.q.front().sequence_length() <= temp){
    outstanding_seqnos_ -= flightMsg.q.front().sequence_length();
    flightMsg.q.pop();
    retransmission_number = 0;
    cur_RTO_ms = initial_RTO_ms_;
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if(flightMsg.q.empty()){
    return;
  }
  // use transmit to resend 
  if(ms_since_last_tick + time_passed_ms >= cur_RTO_ms){
    time_passed_ms = 0;

    if(last_window_size > 0){
      cur_RTO_ms <<= 1;
      retransmission_number++;
    }
    transmit(flightMsg.q.front());
  } else {
    time_passed_ms += ms_since_last_tick;
  }
}
