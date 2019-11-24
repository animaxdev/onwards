#ifndef CMW_Buffer_hh
#define CMW_Buffer_hh 1
#include"quicklz.h"
#include<algorithm>//min
#include<charconv>//from_chars
#include<exception>
#include<initializer_list>
#include<limits>
#if __cplusplus>201703L
#include<span>
#endif
#include<string>
#include<string_view>
#include<type_traits>
static_assert(::std::numeric_limits<unsigned char>::digits==8);
static_assert(::std::numeric_limits<float>::is_iec559,"IEEE754");
#include<stdint.h>
#include<stdio.h>//fopen,snprintf
#include<stdlib.h>//exit
#include<string.h>//memcpy,memmove

#if defined(_MSC_VER)||defined(WIN32)||defined(_WIN32)||defined(__WIN32__)||defined(__CYGWIN__)
#include<winsock2.h>
#include<ws2tcpip.h>
#define CMW_WINDOWS
#define poll WSAPoll
#else
#include<errno.h>
#include<fcntl.h>//fcntl,open
#include<netdb.h>
#include<poll.h>
#include<sys/socket.h>
#include<sys/stat.h>//open
#include<sys/types.h>
#include<syslog.h>
#include<unistd.h>//close,chdir,read,write
#endif

namespace cmw{
inline int getError (){
  return
#ifdef CMW_WINDOWS
   WSAGetLastError();
#else
   errno;
#endif
}

class Failure:public ::std::exception{
  ::std::string s;
public:
  explicit Failure (char const* s):s(s){}
  void operator<< (::std::string_view v){s.append(" "); s.append(v);}
  void operator<< (char const* v){s.append(" "); s.append(v);}
  void operator<< (int i){char b[12]; ::snprintf(b,sizeof b,"%d",i);*this<<b;}
  char const* what ()const noexcept{return s.c_str();}
};

struct Fiasco:Failure{explicit Fiasco (char const* s):Failure(s){}};

template<class E>void apps (E& e){throw e;}
template<class E,class T,class...Ts>void apps (E& e,T t,Ts...ts){
  e<<t; apps(e,ts...);
}

template<class E=Failure,class...T>void raise (char const* s,T...t){
  E e{s}; apps(e,t...);
}

inline void winStart (){
#ifdef CMW_WINDOWS
  WSADATA w;
  if(auto r=::WSAStartup(MAKEWORD(2,2),&w);0!=r)raise("WSAStartup",r);
#endif
}

inline int fromChars (::std::string_view s){
  int res=0;
  ::std::from_chars(s.data(),s.data()+s.size(),res);
  return res;
}

inline void setDirectory (char const* d){
#ifdef CMW_WINDOWS
  if(!::SetCurrentDirectory(d))
#else
  if(::chdir(d)==-1)
#endif
    raise("setDirectory",d,getError());
}

class SendBuffer;
template<class R>class ReceiveBuffer;
class MarshallingInt{
  ::int32_t val;
public:
  MarshallingInt (){}
  explicit MarshallingInt (::int32_t v):val(v){}
  explicit MarshallingInt (::std::string_view v):val(fromChars(v)){}

  //Reads a sequence of bytes in variable-length format and
  //builds a 32 bit integer.
  template<class R>explicit MarshallingInt (ReceiveBuffer<R>& b):val(0){
    ::uint32_t shift=1;
    for(;;){
      ::uint8_t a=b.giveOne();
      val+=(a&127)*shift;
      if((a&128)==0)return;
      shift<<=7;
      val+=shift;
    }
  }

  void operator= (::int32_t r){val=r;}
  auto operator() ()const{return val;}
  void marshal (SendBuffer&)const;
};
inline bool operator== (MarshallingInt l,MarshallingInt r){return l()==r();}
inline bool operator== (MarshallingInt l,::int32_t r){return l()==r;}

#ifdef CMW_WINDOWS
using sockType=SOCKET;
using fileType=HANDLE;
inline DWORD Write (HANDLE h,void const* data,int len){
  DWORD bytesWritten=0;
  if(!WriteFile(h,static_cast<char const*>(data),len,&bytesWritten,nullptr))
    raise("Write",GetLastError());
  return bytesWritten;
}

inline DWORD Read (HANDLE h,void* data,int len){
  DWORD bytesRead=0;
  if(!ReadFile(h,static_cast<char*>(data),len,&bytesRead,nullptr))
    raise("Read",GetLastError());
  return bytesRead;
}
#else
using sockType=int;
using fileType=int;
inline int Write (int fd,void const* data,int len){
  if(int r=::write(fd,data,len);r>=0)return r;
  raise("Write",errno);
}

inline int Read (int fd,void* data,int len){
  int r=::read(fd,data,len);
  if(r>0)return r;
  if(r==0)raise<Fiasco>("Read eof",len);
  if(EAGAIN==errno||EWOULDBLOCK==errno)return 0;
  raise("Read",len,errno);
}

template<class...T>void bail (char const* fmt,T... t)noexcept{
  ::syslog(LOG_ERR,fmt,t...);
  ::exit(EXIT_FAILURE);
}

struct fileWrapper{
  int const d;
  fileWrapper ():d(-2){}
  fileWrapper (char const* name,int flags,mode_t mode=0):
          d(::open(name,flags,mode))
  {if(d<0)raise("fileWrapper",name,errno);}

  ~fileWrapper (){::close(d);}
};

auto getFile=[](char const* n,auto& b){
  b.giveFile(fileWrapper{n,O_WRONLY|O_CREAT|O_TRUNC
                         ,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH}.d);
};

class File{
  char const* nam;
public:
  explicit File (char const* n):nam(n){}

  template<class R>
  explicit File (ReceiveBuffer<R>& b):nam(b.giveStringView().data()){
    getFile(nam,b);
  }

  auto name ()const{return nam;}
};
#endif

inline int pollWrapper (::pollfd* fds,int n,int timeout=-1){
  if(int r=::poll(fds,n,timeout);r>=0)return r;
  raise("poll",getError());
}

template<class T>int setsockWrapper (sockType s,int opt,T t){
  return ::setsockopt(s,SOL_SOCKET,opt,reinterpret_cast<char*>(&t),sizeof t);
}

inline void setRcvTimeout (sockType s,int time){
#ifdef CMW_WINDOWS
  DWORD t=time*1000;
#else
  ::timeval t{time,0};
#endif
  if(setsockWrapper(s,SO_RCVTIMEO,t)!=0)raise("setRcvTimeout",getError());
}

inline int setNonblocking (sockType s){
#ifndef CMW_WINDOWS
  return ::fcntl(s,F_SETFL,O_NONBLOCK);
#endif
}

inline void closeSocket (sockType s){
#ifdef CMW_WINDOWS
  if(::closesocket(s)==SOCKET_ERROR)
#else
  if(::close(s)==-1)
#endif
    raise("closeSocket",getError());
}

inline int preserveError (sockType s){
  auto e=getError();
  closeSocket(s);
  return e;
}

class GetaddrinfoWrapper{
  ::addrinfo* head,*addr;
public:
  GetaddrinfoWrapper (char const* node,char const* port,int type,int flags=0){
    ::addrinfo hints{flags,AF_UNSPEC,type,0,0,0,0,0};
    if(int r=::getaddrinfo(node,port,&hints,&head);r!=0)
      raise("getaddrinfo",::gai_strerror(r));
    addr=head;
  }

  ~GetaddrinfoWrapper (){::freeaddrinfo(head);}
  auto operator() (){return addr;}
  void inc (){if(addr!=nullptr)addr=addr->ai_next;}
  sockType getSock (){
    for(;addr!=nullptr;addr=addr->ai_next){
      if(auto s=::socket(addr->ai_family,addr->ai_socktype,0);-1!=s)return s;
    }
    raise("getaddrinfo getSock");
  }
  GetaddrinfoWrapper (GetaddrinfoWrapper const&)=delete;
  GetaddrinfoWrapper& operator= (GetaddrinfoWrapper)=delete;
};

inline sockType connectWrapper (char const* node,char const* port){
  GetaddrinfoWrapper ai{node,port,SOCK_STREAM};
  auto s=ai.getSock();
  if(0==::connect(s,ai()->ai_addr,ai()->ai_addrlen))return s;
  errno=preserveError(s);
  return -1;
}

inline sockType udpServer (char const* port){
  GetaddrinfoWrapper ai{nullptr,port,SOCK_DGRAM,AI_PASSIVE};
  auto s=ai.getSock();
  if(0==::bind(s,ai()->ai_addr,ai()->ai_addrlen))return s;
  raise("udpServer",preserveError(s));
}

inline sockType tcpServer (char const* port){
  GetaddrinfoWrapper ai{nullptr,port,SOCK_STREAM,AI_PASSIVE};
  auto s=ai.getSock();

  if(int on=1;setsockWrapper(s,SO_REUSEADDR,on)==0
    &&::bind(s,ai()->ai_addr,ai()->ai_addrlen)==0
    &&::listen(s,SOMAXCONN)==0)return s;
  raise("tcpServer",preserveError(s));
}

inline int acceptWrapper(sockType s){
  if(int n=::accept(s,nullptr,nullptr);n>=0)return n;
  auto e=getError();
  if(ECONNABORTED==e)return 0;
  raise("acceptWrapper",e);
}

inline int sockWrite (sockType s,void const* data,int len
                      ,sockaddr const* addr=nullptr,socklen_t toLen=0){
  if(int r=::sendto(s,static_cast<char const*>(data),len,0,addr,toLen);r>0)
    return r;
  auto e=getError();
  if(EAGAIN==e||EWOULDBLOCK==e)return 0;
  raise("sockWrite",s,e);
}

inline int sockRead (sockType s,void* data,int len
                     ,sockaddr* addr,socklen_t* fromLen){
  int r=::recvfrom(s,static_cast<char*>(data),len,0,addr,fromLen);
  if(r>0)return r;
  auto e=getError();
  if(0==r||ECONNRESET==e)raise<Fiasco>("sockRead eof",s,len,e);
  if(EAGAIN==e||EWOULDBLOCK==e
#ifdef CMW_WINDOWS
     ||WSAETIMEDOUT==e
#endif
  )return 0;
  raise("sockRead",s,len,e);
}

struct SameFormat{
  template<template<class> class B,class U>
  static void read (B<SameFormat>& b,U& data){b.give(&data,sizeof(U));}

  template<template<class> class B,class U>
  static void readBlock (B<SameFormat>& b,U* data,int elements)
  {b.give(data,elements*sizeof(U));}
};

struct LeastSignificantFirst{
  template<template<class> class B>
  static void read (B<LeastSignificantFirst>& b,::uint16_t& val)
  {for(::uint8_t c=0;c<sizeof val;++c)val|=b.giveOne()<<8*c;}

  template<template<class> class B>
  static void read (B<LeastSignificantFirst>& b,::uint32_t& val)
  {for(::uint8_t c=0;c<sizeof val;++c)val|=b.giveOne()<<8*c;}

  template<template<class> class B>
  static void read (B<LeastSignificantFirst>& b,::uint64_t& val)
  {for(::uint8_t c=0;c<sizeof val;++c)val|=b.giveOne()<<8*c;}

  template<template<class> class B>
  static void read (B<LeastSignificantFirst>& b,float& f){
    ::uint32_t tmp;
    read(b,tmp);
    ::memcpy(&f,&tmp,sizeof f);
  }

  template<template<class> class B>
  static void read (B<LeastSignificantFirst>& b,double& d){
    ::uint64_t tmp;
    read(b,tmp);
    ::memcpy(&d,&tmp,sizeof d);
  }

  template<template<class> class B,class U>
  static void readBlock (B<LeastSignificantFirst>& b,U* data,int elements){
    for(int i=0;i<elements;++i){*(data+i)=b.template give<U>();}
  }
};

struct MostSignificantFirst{
  template<template<class> class B>
  static void read (B<MostSignificantFirst>& b,::uint16_t& val)
  {for(::uint8_t c=sizeof(val)-1;c>=0;--c)val|=b.giveOne()<<8*c;}

  template<template<class> class B>
  static void read (B<MostSignificantFirst>& b,::uint32_t& val)
  {for(::uint8_t c=sizeof(val)-1;c>=0;--c)val|=b.giveOne()<<8*c;}

  template<template<class> class B>
  static void read (B<MostSignificantFirst>& b,::uint64_t& val)
  {for(::uint8_t c=sizeof(val)-1;c>=0;--c)val|=b.giveOne()<<8*c;}

  template<template<class> class B>
  static void read (B<MostSignificantFirst>& b,float& f){
    ::uint32_t tmp;
    read(b,tmp);
    ::memcpy(&f,&tmp,sizeof f);
  }

  template<template<class> class B>
  static void read (B<MostSignificantFirst>& b,double& d){
    ::uint64_t tmp;
    read(b,tmp);
    ::memcpy(&d,&tmp,sizeof d);
  }

  template<template<class> class B,class U>
  static void readBlock (B<MostSignificantFirst>& b,U* data,int elements){
    for(int i=0;i<elements;++i){*(data+i)=b.template give<U>();}
  }
};

template<class R> class ReceiveBuffer{
  int msgLength=0;
  int subTotal=0;
protected:
  int rindex=0;
  int packetLength;
  char* const rbuf;
public:
  ReceiveBuffer (char* addr,int bytes):packetLength(bytes),rbuf(addr){}

  void checkLen (int n){
    if(n>msgLength-rindex)raise("ReceiveBuffer checkLen",n,msgLength,rindex);
  }

  void give (void* address,int len){
    checkLen(len);
    ::memcpy(address,rbuf+subTotal+rindex,len);
    rindex+=len;
  }

  char giveOne (){
    checkLen(1);
    return rbuf[subTotal+rindex++];
  }

  template<class T>T give (){
    T t;
    if constexpr(sizeof(T)==1)give(&t,1);
    else R::read(*this,t);
    return t;
  }

  bool nextMessage (){
    subTotal+=msgLength;
    if(subTotal<packetLength){
      rindex=0;
      msgLength=sizeof(::int32_t);
      msgLength=give<::uint32_t>();
      return true;
    }
    return false;
  }

  bool update (){
    msgLength=subTotal=0;
    return nextMessage();
  }

  template<class T>void giveBlock (T* data,unsigned int elements){
    if constexpr(sizeof(T)==1)give(data,elements);
    else R::readBlock(*this,data,elements);
  }

  void giveFile (fileType d){
    int sz=give<::uint32_t>();
    checkLen(sz);
    while(sz>0){
      int r=Write(d,rbuf+subTotal+rindex,sz);
      sz-=r;
      rindex+=r;
    }
  }

#if __cplusplus>201703L
  template<class T>auto giveSpan (){
    static_assert(::std::is_arithmetic_v<T>);
    ::int32_t sz=give<::uint32_t>();
    ::int32_t serLen=sizeof(T)*sz;
    checkLen(serLen);
    ::std::span<T> s(reinterpret_cast<T*>(rbuf+subTotal+rindex),sz);
    rindex+=serLen;
    return s;
  }
#endif

  auto giveStringView (){
    MarshallingInt len{*this};
    checkLen(len());
    ::std::string_view s(rbuf+subTotal+rindex,len());
    rindex+=len();
    return s;
  }

  template<class T>void giveIlist (T& lst){
    for(int c=give<::uint32_t>();c>0;--c)
      lst.push_back(*T::value_type::buildPolyInstance(*this));
  }

  template<class T>void giveRbtree (T& rbt){
    auto endIt=rbt.end();
    for(int c=give<::uint32_t>();c>0;--c)
      rbt.insert_unique(endIt,*T::value_type::buildPolyInstance(*this));
  }
};

template<class T,class R>T give (ReceiveBuffer<R>& b)
{return b.template give<T>();}

template<class R>bool giveBool (ReceiveBuffer<R>& b){
  switch(b.giveOne()){
    case 0:return false;
    case 1:return true;
    default:raise("giveBool");
  }
}

class SendBuffer{
  SendBuffer (SendBuffer const&);
  SendBuffer& operator= (SendBuffer);
  ::int32_t savedSize=0;
protected:
  int index=0;
  int const bufsize;
  unsigned char* const buf;
public:
  sockType sock_=-1;

  SendBuffer (unsigned char* addr,int sz):bufsize(sz),buf(addr){}

  int reserveBytes (int n){
    if(n>bufsize-index)raise("SendBuffer checkSpace",n,index);
    auto i=index;
    index+=n;
    return i;
  }

  void receive (void const* data,int size){
    auto prev=reserveBytes(size);
    ::memcpy(buf+prev,data,size);
  }

  template<class T>void receive (T t){
    static_assert(::std::is_arithmetic_v<T>||::std::is_enum_v<T>);
    receive(&t,sizeof t);
  }

  template<class T>T receive (int where,T t){
    static_assert(::std::is_arithmetic_v<T>);
    ::memcpy(buf+where,&t,sizeof t);
    return t;
  }

  void fillInSize (::int32_t max){
    ::int32_t marshalledBytes=index-savedSize;
    if(marshalledBytes>max)raise("fillInSize",max);
    receive(savedSize,marshalledBytes);
    savedSize=index;
  }

  void reset (){savedSize=index=0;}
  void rollback (){index=savedSize;}

  void receiveFile (fileType d,::int32_t sz){
    receive(sz);
    auto prev=reserveBytes(sz);
    if(Read(d,buf+prev,sz)!=sz)raise("SendBuffer receiveFile");
  }

  bool flush (){
    int const bytes=sockWrite(sock_,buf,index);
    if(bytes==index){reset();return true;}

    index-=bytes;
    savedSize=index;
    ::memmove(buf,buf+bytes,index);
    return false;
  }

  //UDP-friendly alternative to flush
  void send (::sockaddr* addr=nullptr,::socklen_t len=0)
  {sockWrite(sock_,buf,index,addr,len);}

  unsigned char* data (){return buf;}
  int getIndex (){return index;}
  int getSize (){return bufsize;}
  template<class...T>void receiveMulti (char const*,T...);
};

inline void receiveBool (SendBuffer&b,bool bl){b.receive<unsigned char>(bl);}

inline void receive (SendBuffer& b,::std::string_view s,int a=0){
  MarshallingInt(s.size()+a).marshal(b);
  b.receive(s.data(),s.size()+a);
}

using stringPlus=::std::initializer_list<::std::string_view>;
inline void receive (SendBuffer& b,stringPlus lst){
  ::int32_t t=0;
  for(auto s:lst)t+=s.size();
  MarshallingInt{t}.marshal(b);
  for(auto s:lst)b.receive(s.data(),s.size());//Use low-level receive
}

template<template<class...>class C,class T,class...U>
void receiveBlock (SendBuffer& b,C<T,U...>const& c){
  ::int32_t n=c.size();
  b.receive(n);
  if constexpr(::std::is_arithmetic_v<T>){
    if(n>0)b.receive(c.data(),n*sizeof(T));
  }else if constexpr(::std::is_pointer_v<T>)for(auto e:c)e->marshal(b);
  else for(auto const& e:c)e.marshal(b);
}

//Encode integer into variable-length format.
inline void MarshallingInt::marshal (SendBuffer& b)const{
  ::uint32_t n=val;
  for(;;){
    ::uint8_t a=n&127;
    n>>=7;
    if(0==n){b.receive(a);return;}
    b.receive(a|=128);
    --n;
  }
}

auto constexpr udp_packet_max=1280;
template<class R,int N=udp_packet_max>
struct BufferStack:SendBuffer,ReceiveBuffer<R>{
private:
  unsigned char ar[N];
public:
  BufferStack ():SendBuffer(ar,N),ReceiveBuffer<R>((char*)ar,0){}
  BufferStack (int s):BufferStack(){sock_=s;}

  bool getPacket (::sockaddr* addr=nullptr,::socklen_t* len=nullptr){
    this->packetLength=sockRead(sock_,ar,N,addr,len);
    return this->update();
  }
};

template<class T>void reset (T* p){::memset(p,0,sizeof(T));}

struct SendBufferHeap:SendBuffer{
  SendBufferHeap (int sz):SendBuffer(new unsigned char[sz],sz){}
  ~SendBufferHeap (){delete[]buf;}
};

template<class R>struct BufferCompressed:SendBufferHeap,ReceiveBuffer<R>{
private:
  ::qlz_state_compress* compress;
  int const compSize;
  int compPacketSize;
  int compIndex=0;
  char* compBuf;
  bool kosher=true;

  ::qlz_state_decompress* decomp;
  int bytesRead=0;
  char* compressedStart;

  bool doFlush (){
    int const bytes=Write(sock_,compBuf,compIndex);
    if(bytes==compIndex){compIndex=0;return true;}
    compIndex-=bytes;
    ::memmove(compBuf,compBuf+bytes,compIndex);
    return false;
  }
public:
  BufferCompressed (int sz,int d):SendBufferHeap(sz),ReceiveBuffer<R>(new char[sz],0)
              ,compress(nullptr),compSize(sz+(sz>>3)+400),compBuf(nullptr)
              ,decomp(nullptr){(void)d;}

  explicit BufferCompressed (int sz):BufferCompressed(sz,0){
    compress=new ::qlz_state_compress();
    compBuf=new char[compSize];
    decomp=new ::qlz_state_decompress();
  }

  ~BufferCompressed (){
    delete[]rbuf;
    delete compress;
    delete[]compBuf;
    delete decomp;
  }

  bool flush (){
    bool rc=true;
    if(compIndex>0)rc=doFlush();

    if(index>0){
      if(index+(index>>3)+400>compSize-compIndex)
        raise("Not enough room in compressed buf");
      compIndex+=::qlz_compress(buf,compBuf+compIndex,index,compress);
      reset();
      if(rc)rc=doFlush();
    }
    return rc;
  }

  void compressedReset (){
    reset();
    ::cmw::reset(compress);
    ::cmw::reset(decomp);
    compIndex=bytesRead=0;
    kosher=true;
    closeSocket(sock_);
  }

  using ReceiveBuffer<R>::rbuf;
  bool gotPacket (){
    if(kosher){
      if(bytesRead<9){
        bytesRead+=Read(sock_,rbuf+bytesRead,9-bytesRead);
        if(bytesRead<9)return false;
        if((compPacketSize=::qlz_size_compressed(rbuf))>bufsize||
           (this->packetLength=::qlz_size_decompressed(rbuf))>bufsize){
          kosher=false;
          raise("gotPacket too big",compPacketSize,this->packetLength,bufsize);
        }
        compressedStart=rbuf+bufsize-compPacketSize;
        ::memmove(compressedStart,rbuf,9);
      }
      bytesRead+=Read(sock_,compressedStart+bytesRead,compPacketSize-bytesRead);
      if(bytesRead<compPacketSize)return false;
      ::qlz_decompress(compressedStart,rbuf,decomp);
      bytesRead=0;
      this->update();
      return true;
    }
    bytesRead+=Read(sock_,rbuf,::std::min(bufsize,compPacketSize-bytesRead));
    if(bytesRead==compPacketSize){
      kosher=true;
      bytesRead=0;
    }
    return false;
  }
};

template<int N>class FixedString{
  MarshallingInt len;
  char str[N];
public:
  FixedString (){}

  explicit FixedString (::std::string_view s):len(s.size()){
    if(len()>=N)raise("FixedString ctor");
    ::memcpy(str,s.data(),len());
    str[len()]=0;
  }

  template<class R>explicit FixedString (ReceiveBuffer<R>& b):len(b){
    if(len()>=N)raise("FixedString stream ctor");
    b.give(str,len());
    str[len()]=0;
  }

  void marshal (SendBuffer& b)const{
    len.marshal(b);
    b.receive(str,len());
  }

  int bytesAvailable (){return N-(len()+1);}

  char* operator() (){return str;}
  char const* c_str ()const{return str;}
  char operator[] (int i)const{return str[i];}
};
using FixedString60=FixedString<60>;
using FixedString120=FixedString<120>;

struct FILEwrapper{
  ::FILE* const hndl;
  char line[120];

  FILEwrapper (char const* fn,char const* mode):hndl(::fopen(fn,mode))
  {if(nullptr==hndl)raise("FILEwrapper",fn,mode,errno);}
  char* fgets (){return ::fgets(line,sizeof line,hndl);}
  ~FILEwrapper (){::fclose(hndl);}
};
}
#endif
