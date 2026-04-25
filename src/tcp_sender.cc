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

TCPSenderMessage TCPSender::StreamToMsg(uint64_t available_capacity) {
  bool SYN = last_n == 0;

  // If we don't even have space for a SYN, return empty
  if (available_capacity < SYN) {
    return {};
  }

  uint64_t payload_capacity = available_capacity - SYN;
  uint64_t max_payload = TCPConfig::MAX_PAYLOAD_SIZE;
  uint64_t minSize = std::min({max_payload, reader().bytes_buffered(), payload_capacity});

  std::string payload = "";
  while(payload.size() < minSize) {
    std::string_view view = reader().peek();
    if(view.empty()) break;
    std::string_view chunk = view.substr(0, minSize - payload.size());
    payload += chunk;
    reader().pop(chunk.size());
  }

  bool FIN = false;
  // Only set FIN if stream is done, buffer is empty, AND we have room in the window
  if(!flightMsg.fin_sent && reader().is_finished() && reader().bytes_buffered() == 0) {
    if(available_capacity - SYN - payload.size() >= 1) {
      FIN = true;
      flightMsg.fin_sent = true; 
    }
  }

  Wrap32 seqno = Wrap32::wrap(last_n, isn_);
  
  TCPSenderMessage toSend = {
    .seqno = seqno,
    .SYN = SYN,
    .payload = payload,
    .FIN = FIN,
    .RST = input_.has_error(),
  };

  last_n += toSend.sequence_length();
  outstanding_seqnos_ += toSend.sequence_length();
  return toSend;
}

void TCPSender::push( const TransmitFunction& transmit ) {
  while(true) {
    // Zero-window probing: treat window as 1 if 0
    uint64_t effective_window = (last_window_size == 0) ? 1 : last_window_size;
    uint64_t win_end = flightMsg.acked_seqno_ + effective_window;
    
    // Calculate how much space is actually left in the sliding window
    uint64_t available_capacity = (win_end > last_n) ? (win_end - last_n) : 0;

    if (available_capacity == 0) {
      break;
    }

    TCPSenderMessage payload = StreamToMsg(available_capacity);

    if(payload.sequence_length() == 0) {
      break;
    }

    flightMsg.q.push(payload);
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

void TCPSender::receive( const TCPReceiverMessage& msg ) {
  if(msg.RST){
    input_.set_error(); 
    return;
  } 

  if(!msg.ackno.has_value()){
    // First message before SYN is acked
    last_window_size = msg.window_size;
    return;
  }

  uint64_t abs_ackno = msg.ackno.value().unwrap(isn_, last_n);

  // Ignore impossible ACKs (acking data we haven't sent)
  if (abs_ackno > last_n) {
    return;
  }

  last_window_size = msg.window_size;
  flightMsg.acked_seqno_ = std::max(flightMsg.acked_seqno_, abs_ackno);

  bool newly_acked = false;
  while(!flightMsg.q.empty()) {
    auto& front_msg = flightMsg.q.front();
    uint64_t msg_seq = front_msg.seqno.unwrap(isn_, last_n);
    
    if (msg_seq + front_msg.sequence_length() <= flightMsg.acked_seqno_) {
      outstanding_seqnos_ -= front_msg.sequence_length();
      flightMsg.q.pop();
      newly_acked = true;
    } else {
      break; // Message is only partially acked or not acked at all
    }
  }

  if (newly_acked) {
    retransmission_number = 0;
    cur_RTO_ms = initial_RTO_ms_;
    time_passed_ms = 0; // The crucial missing timer reset
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit ) {
  if(flightMsg.q.empty()){
    return;
  }

  time_passed_ms += ms_since_last_tick;

  if(time_passed_ms >= cur_RTO_ms){
    time_passed_ms = 0;

    // Only apply exponential backoff if the receiver's window isn't 0
    if(last_window_size > 0){ 
      cur_RTO_ms <<= 1;
      retransmission_number++;
    }
    transmit(flightMsg.q.front());
  } 
}
