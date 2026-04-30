#include <iostream>

#include "arp_message.hh"
#include "debug.hh"
#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address)
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
  , ipToEthMap()
  , arpPending()
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( InternetDatagram dgram, const Address& next_hop )
{
  // If dest Ethernet address is known 
  if(ipToEthMap.find(next_hop) != ipToEthMap.end()){
    // -> Create ethernet frame
    // create header
    EthernetHeader header; 
    // set source + dest address
    header.type = EthernetHeader::TYPE_IPv4;
    header.src = ethernet_address_;
    header.dst = get<0>(ipToEthMap[next_hop]);

    // create submission
    EthernetFrame submission;
    submission.header = header;
    // Payload serialized datagram
    submission.payload = ::serialize(dgram);
    // submit
    transmit(submission);
  } else {

    // broadcast ARP request (with timer of last 5 seconds)
    // -> queue ip datagram
    // set source + dest address
    EthernetHeader header; 
    header.type = EthernetHeader::TYPE_ARP;
    header.src = ethernet_address_;
    header.dst = ETHERNET_BROADCAST;

    EthernetFrame submission;
    submission.header = header;

    // create ARPMessage
    ARPMessage msg; 
    msg.opcode = ARPMessage::OPCODE_REQUEST;
    msg.sender_ethernet_address = ethernet_address_;
    msg.target_ip_address = next_hop.ipv4_numeric(); 
    msg.sender_ip_address = ip_address_.ipv4_numeric(); 

    submission.payload = ::serialize(msg);

    // no queue yet created for speicfic hop
    if(arpPending.find(next_hop) == arpPending.end()){
      // no transmit made yet
      ArpPending arp;
      arp.time = 5000;
      arp.q.push(dgram);
      arpPending[next_hop] = arp;
      transmit(submission);
    } else {
      // add valid datagram
      arpPending[next_hop].q.push(dgram);
    }
  } 
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  // if IPv4 parse as InternetDatagram + success
  // -> push datagram on the datagrams_received_ queue
  if(frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST){
    // ignore irrelevant frame
    return;
  }

  if(frame.header.type == EthernetHeader::TYPE_IPv4){
    // parse the message
    InternetDatagram internet_datagram;
    if(parse(internet_datagram, frame.payload)){
      datagrams_received().push(internet_datagram);
    }

    return ;
  } 
  
  if (frame.header.type == EthernetHeader::TYPE_ARP){
    ARPMessage arpMsg;
    if(parse(arpMsg, frame.payload)){
      Address orig = Address::from_ipv4_numeric(arpMsg.sender_ip_address);
      ipToEthMap[orig] = {arpMsg.sender_ethernet_address, 30000};


      if(arpPending.find(orig) != arpPending.end()){

        // if arp response
        InternetDatagram flush; 
        while(arpPending[orig].q.size() != 0){
          auto dgram = arpPending[orig].q.front();

          EthernetHeader header; 
          // set source + dest address
          header.type = EthernetHeader::TYPE_IPv4;
          header.src = ethernet_address_;
          header.dst = arpMsg.sender_ethernet_address;
          
          // create submission
          EthernetFrame submission;
          submission.header = header;
          // Payload serialized datagram
          submission.payload = ::serialize(dgram);
          // submit
          transmit(submission);
          arpPending[orig].q.pop();
        }

        arpPending.erase(orig);
      }

      // check if it wants cur machine ip
      if(Address::from_ipv4_numeric(arpMsg.target_ip_address) != ip_address_){
        // doesn't want this machine
        return ;
      }

      if(arpMsg.opcode == ARPMessage::OPCODE_REQUEST){
      // handle arp request
        EthernetHeader header; 
        header.type = EthernetHeader::TYPE_ARP;
        header.src = ethernet_address_;
        header.dst = arpMsg.sender_ethernet_address;

        EthernetFrame submission;
        submission.header = header;

        // create ARPMessage
        ARPMessage msg; 
        msg.opcode = ARPMessage::OPCODE_REPLY;
        msg.sender_ethernet_address = ethernet_address_;
        msg.target_ip_address = arpMsg.sender_ip_address; 
        msg.sender_ip_address = ip_address_.ipv4_numeric(); 
        msg.target_ethernet_address = arpMsg.sender_ethernet_address; 

        submission.payload = ::serialize(msg);

        transmit(submission);
      }
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  
  for (auto it = ipToEthMap.begin(); it != ipToEthMap.end(); ) {
    if(get<1>(it->second) <= ms_since_last_tick){
      it = ipToEthMap.erase(it);
    } else {
      get<1>(it->second) -= ms_since_last_tick; 
      ++it;
    }
  }

  for (auto it = arpPending.begin(); it != arpPending.end(); ) {
    if(it->second.time <= ms_since_last_tick){
      it = arpPending.erase(it);
    } else {
      it->second.time -= ms_since_last_tick; 
      ++it;
    }
  }
}
