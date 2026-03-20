#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if(message.RST){
    reader().set_error();
    return;
    // Do something to reset here
  }

  // add the initial value 
  if(message.SYN){
    initial = message.seqno;
  }

  // return nothing if no initial is set
  if(!initial){ return; }

  // add data to stream
  uint64_t abs_seq = (message.seqno).unwrap(*initial, writer().bytes_pushed() + 1);
  uint64_t stream_idx = abs_seq + message.SYN - 1;
  reassembler_.insert(stream_idx, message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  uint16_t max= ~0;
  if(initial){
    uint64_t abs_ack = writer().bytes_pushed() + 1 + (writer().is_closed() ? 1: 0);
    Wrap32 ack = Wrap32::wrap(abs_ack, *initial);
    TCPReceiverMessage back = {
      .ackno = ack,
      .window_size = (max > writer().available_capacity()) ? (uint16_t)writer().available_capacity() : max,
      .RST = false
    };
    return back;
  }

  TCPReceiverMessage back = {
      .window_size = (max > writer().available_capacity()) ? (uint16_t)writer().available_capacity() : max,
      .RST = false
  };
  return back;

}

