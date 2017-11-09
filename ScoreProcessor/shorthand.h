//THIS FILE SHOULD BE INCLUDED LAST
#ifdef _MEMORY_
#ifndef MEMORY_SHORTHAND
#define MEMORY_SHORTHAND
template<typename T>
using uptr=::std::unique_ptr<T>;
template<typename T>
using sptr=::std::shared_ptr<T>;
template<typename T>
using wptr=::std::weak_ptr<T>;
#endif //MEMORY_SHORTHAND
#endif //_MEMORY_

#ifndef NUMBER_SHORTHAND
#define NUMBER_SHORTHAND
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned long long ulonglong;	
typedef signed int sint;
typedef signed long slong;
typedef signed short sshort;
typedef signed char schar;
typedef signed long long slonglong;	
#endif //signed_SHORTHAND

#ifndef CAST_SHORTHAND
#define CAST_SHORTHAND
#define scast static_cast
#define dcast dynamic_cast;
#define rcast reinterpret_cast;
#define ccast const_cast;
#endif //CAST_SHORTHAND