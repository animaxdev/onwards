//   This program receives messages sent by sendexample.
//

#include <platforms.hh>
#include <ReceiveBufferStack.hh>
#include <udp_stuff.hh>
#include "zz.receive_example_messages.hh"
#include "plf_colony.h"

#include <array>
#include <iostream>
#include <set>
#include <string>
#include <vector>

int main()
{
  try {
    windows_start();
    auto sd = ::cmw::udp_server("12345");

    for (;;){
      cmw::ReceiveBufferStack<cmw::SameFormat> buffer(sd);
      auto msgid = buffer.Give<messageid_t>();
      std::cout << "Message id: " << static_cast<unsigned>(msgid) << '\n';
      switch (msgid) {
      case messageid1:
      {
        std::vector<int32_t> vec;
        std::string str;
        receive_example_messages::Give(buffer,vec,str);
        for (auto val:vec) {std::cout<<val<<" ";}
        std::cout << '\n' << str;
      }
      break;

      case messageid2:
      {
        std::set<int32_t> iset;
        receive_example_messages::Give(buffer,iset);
        for (auto val:iset) {std::cout<<val<<" ";}
      }
      break;

      case messageid3:
      {
        ::std::array<std::array<float,2>, 3> a;
        receive_example_messages::Give(buffer,a);
        for (auto subarray:a) {
          for (auto val:subarray) {std::cout<<val<<" ";}
          std::cout << std::endl;
        }
      }
      break;

      case messageid4:
      {
        ::plf::colony<::std::string> clny;
        receive_example_messages::Give(buffer,clny);
        for (auto val:clny) {std::cout<<val<<" ";}
      }
      break;

      default:
        std::cout << "Unexpected message id: " << msgid;
      }
      std::cout << std::endl;
    }
    return 1;
  } catch (std::exception const& ex) {
    std::cout << "failure: " << ex.what() << std::endl;
  }
}

