#pragma once

#include "exception.hh"
#include "network_interface.hh"

#include <optional>


struct Path {
  uint32_t route_prefix;
  uint8_t prefix_length;
  std::optional<Address> next_hop;
  size_t interface_num;

  Path(
    uint32_t route_prefix_,
    uint8_t prefix_length_,
    std::optional<Address> next_hop_,
    size_t interface_num_
  ): route_prefix(route_prefix_), prefix_length(prefix_length_), next_hop(next_hop_), interface_num(interface_num_)
  {};

  bool match(InternetDatagram id){
      
    int inverse_length = 32 - prefix_length;
    if(prefix_length == 0){
      return true;
    } else if(id.header.dst >> inverse_length == route_prefix >> inverse_length){
      return true;
    }
    return false;
  }
};

// \brief A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.
class Router
{
public:
  // Add an interface to the router
  // \param[in] interface an already-constructed network interface
  // \returns The index of the interface after it has been added to the router
  size_t add_interface( std::shared_ptr<NetworkInterface> interface )
  {
    interfaces_.push_back( notnull( "add_interface", std::move( interface ) ) );
    return interfaces_.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface( const size_t N ) { return interfaces_.at( N ); }

  // Add a route (a forwarding rule)
  void add_route( uint32_t route_prefix,
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  // Route packets between the interfaces
  void route();

private:
  // The router's collection of network interfaces
  std::vector<std::shared_ptr<NetworkInterface>> interfaces_ {};
  std::vector<Path> paths_{};
};
