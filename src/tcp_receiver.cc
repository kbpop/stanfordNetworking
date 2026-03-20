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
    initial = message.seqno.unwrap(message.seqno, 0);
  }

  // return nothing if no initial is set
  if(!initial){ return; }

  // add data to stream
  reassembler_.insert(*initial, message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  if(initial){
    uint32_t ack = *initial + writer().bytes_pushed() + 1;
    TCPReceiverMessage back = {
      .ackno = Wrap32{ack},
      .window_size = (uint16_t)writer().available_capacity(),
      .RST = false
    };
    return back;
  }

  TCPReceiverMessage back = {
      .window_size = (uint16_t)writer().available_capacity(),
      .RST = false
  };
  return back;

}

