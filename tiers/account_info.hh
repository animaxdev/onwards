#pragma once
#include<marshalling_integer.hh>
#include<fixed_string.hh>

namespace cmw{
class SendBuffer;
template<class R> class ReceiveBuffer;
}

struct cmw_account{
  ::cmw::marshalling_integer number;
  ::cmw::fixed_string_60 password;

  cmw_account (int num,char* pass):number(num),password(pass){}

  template<class R>
  explicit cmw_account (::cmw::ReceiveBuffer<R>&);

  void MarshalMemberData (::cmw::SendBuffer&)const;
  void Marshal (::cmw::SendBuffer& buf,bool=false)const
  {MarshalMemberData(buf);}
};
