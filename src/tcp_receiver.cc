#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if(message.RST){
    // Do something to reset here
  }

  // add the initial value 
  if(message.SYN){
    initial = message.seqno;
  }

  // return nothing if no initial is set
  if(!initial){ return; }

  // add data to stream
  reassembler_.insert((message.seqno).unwrap(*initial, writer().bytes_pushed()), message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  uint16_t max= ~0;
  if(initial){
    Wrap32 ack((uint64_t)((*initial).unwrap(*initial, writer().bytes_pushed() + 1)), initial);
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

