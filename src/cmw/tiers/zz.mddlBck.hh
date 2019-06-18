#ifndef zz_mddlBck_hh
#define zz_mddlBck_hh 1
//Code generated by the C++ Middleware Writer version 1.14.
inline void
cmwAccount::MarshalMembers (::cmw::SendBuffer& buf)const{
  number.Marshal(buf);
  password.Marshal(buf);
}

namespace mddlBck{
int32_t Mar (::cmw::SendBuffer& buf
         ,::std::vector<cmwAccount> const& a
         ,::int32_t b){
  ReceiveGroup(buf,a);
  buf.Receive(b);
  return 10000;
}

int32_t Mar (::cmw::SendBuffer& buf
         ,cmwRequest const& a){
  a.Marshal(buf);
  return 700000;
}

int32_t Mar (::cmw::SendBuffer& buf){
  return 10000;
}

template<messageID id,class...T>
void Marshal (::cmw::SendBuffer& buf,T&&...t)try{
  buf.ReserveBytes(4);
  buf.Receive(static_cast<uint8_t>(id));
  buf.FillInSize(Mar(buf,t...));
}catch(...){buf.Rollback();throw;}
}

namespace mddlFrnt{
template<bool res>
void Marshal (::cmw::SendBuffer& buf
         ,::cmw::stringPlus const& a={}
         ,::int8_t b={})try{
  buf.ReserveBytes(4);
  Receive(buf,res);
  if(!res){
    Receive(buf,a);
    buf.Receive(b);
  }
  buf.FillInSize(udp_packet_max);
}catch(...){buf.Rollback();throw;}
}
#endif