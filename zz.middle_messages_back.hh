#pragma once
// Code generated by the C++ Middleware Writer version 1.13.
#include"message_ids.hh"
#include"account_info.hh"
#include"marshalling_integer.hh"
#include"vector"
#include"SendBuffer.hh"

namespace middle_messages_back{
inline void Marshal (::cmw::SendBuffer& buf
         ,messageid_t const& az1
         ,::std::vector<cmw_account> const& az2
         ,int32_t max_length=10000){
  try{
    buf.ReserveBytes(4);
    buf.Receive(az1);
    buf.ReceiveGroup(az2);
    buf.FillInSize(max_length);
  }catch(...){buf.Rollback();throw;}
}

inline void Marshal (::cmw::SendBuffer& buf
         ,messageid_t const& az1
         ,::cmw::marshalling_integer const& az2
         ,request_generator const& az3
         ,int32_t max_length=10000){
  try{
    buf.ReserveBytes(4);
    buf.Receive(az1);
    az2.Marshal(buf);
    az3.Marshal(buf);
    buf.FillInSize(max_length);
  }catch(...){buf.Rollback();throw;}
}

inline void Marshal (::cmw::SendBuffer& buf
         ,messageid_t const& az1
         ,int32_t max_length=10000){
  try{
    buf.ReserveBytes(4);
    buf.Receive(az1);
    buf.FillInSize(max_length);
  }catch(...){buf.Rollback();throw;}
}
}

inline void
cmw_account::MarshalMemberData (::cmw::SendBuffer& buf) const{
  number.Marshal(buf);
  buf.Receive(password);
}
