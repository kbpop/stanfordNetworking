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
  reassembler_.insert((*initial).unwrap(*initial, 0), message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  uint16_t max= ~0;
  if(initial){
    uint32_t ack = (*initial).unwrap(*initial, writer().bytes_pushed() + 1);
    TCPReceiverMessage back = {
      .ackno = Wrap32{ack},
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

