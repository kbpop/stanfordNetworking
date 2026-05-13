#include "router.hh"
#include "debug.hh"

#include <iostream>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const std::optional<Address> next_hop,
                        const size_t interface_num )
{

  // adds to the path route 
  paths_.push_back(Path(
    route_prefix,
    prefix_length,
    next_hop,
    interface_num));
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{

  for(auto inter: interfaces_){

    while(!inter.get()->datagrams_received().empty()){
      InternetDatagram id = inter.get()->datagrams_received().front();

      // check if ttl is 0 or 1
      if(id.header.ttl <= 1){
        inter.get()->datagrams_received().pop();
        continue;
      } else {
        id.header.ttl--;
        id.header.compute_checksum();
      }

      int prefix_length = 0;
      int index = -1;
      for(unsigned int i = 0; i < paths_.size(); i++ ){
        // Check if matches AND the current prefix is longer than the previous one
        if(paths_[i].match(id) && paths_[i].prefix_length >= prefix_length){
          prefix_length = paths_[i].prefix_length;
          index = i; 
        }
      }

      // check if no route was found
      if(index == -1){
        // skip
        inter.get()->datagrams_received().pop();
        continue;
      } 

      // route to the given index
      if(paths_[index].next_hop.has_value()){
        interface(paths_[index].interface_num)->send_datagram(id, paths_[index].next_hop.value());
      } else {
        interface(paths_[index].interface_num)->send_datagram(id, Address::from_ipv4_numeric(id.header.dst));
      }
      inter.get()->datagrams_received().pop();
    }
  }

  return;
}
