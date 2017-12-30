#pragma once
//Code generated by the C++ Middleware Writer version 1.14.
#include<SendBuffer.hh>
#include"account.hh"
#include<vector>

namespace middleBack{
void Marshal (::cmw::SendBuffer& buf
         ,message_id_8 const& a
         ,::std::vector<cmwAccount> const& b){
  try{
    buf.ReserveBytes(4);
    buf.Receive(a);
    buf.ReceiveGroup(b);
    buf.FillInSize(10000);
  }catch(...){buf.Rollback();throw;}
}

inline void Marshal (::cmw::SendBuffer& buf
         ,message_id_8 const& a
         ,cmwRequest const& b){
  try{
    buf.ReserveBytes(4);
    buf.Receive(a);
    b.Marshal(buf);
    buf.FillInSize(700000);
  }catch(...){buf.Rollback();throw;}
}

inline void Marshal (::cmw::SendBuffer& buf
         ,message_id_8 const& a){
  try{
    buf.ReserveBytes(4);
    buf.Receive(a);
    buf.FillInSize(10000);
  }catch(...){buf.Rollback();throw;}
}
}

inline void
cmwAccount::MarshalMemberData (::cmw::SendBuffer& buf)const{
  number.Marshal(buf);
  password.Marshal(buf);
}
