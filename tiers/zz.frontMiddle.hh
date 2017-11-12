#pragma once
//Code generated by the C++ Middleware Writer version 1.14.
#include<SendBuffer.hh>
#include"udp_stuff.hh"
#include<marshalling_integer.hh>

namespace frontMiddle{
void Marshal (::cmw::SendBuffer& buf
         ,cmw::marshalling_integer const& a
         , const char* b){
  try{
    buf.ReserveBytes(4);
    a.Marshal(buf);
    buf.Receive(b);
    buf.FillInSize(cmw::udp_packet_max);
  }catch(...){buf.Rollback();throw;}
}
}
