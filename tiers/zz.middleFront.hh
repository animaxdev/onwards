#pragma once
//Code generated by the C++ Middleware Writer version 1.14.
#include<SendBuffer.hh>
#include"udp_stuff.hh"

namespace middleFront{
void Marshal (::cmw::SendBuffer& buf
         ,bool a
         ,cmw::stringPlus const& b={}){
  try{
    buf.ReserveBytes(4);
    buf.Receive(a);
    if(a)goto rt;
    buf.Receive(b);
rt:
    buf.FillInSize(cmw::udp_packet_max);
  }catch(...){buf.Rollback();throw;}
}
}
