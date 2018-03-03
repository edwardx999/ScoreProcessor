//THIS FILE SHOULD BE INCLUDED LAST
#ifdef _MEMORY_
#ifndef MEMORY_SHORTHAND
#define MEMORY_SHORTHAND
namespace std {
	template<typename T>
	using uptr=unique_ptr<T>;
	template<typename T>
	using sptr=shared_ptr<T>;
	template<typename T>
	using wptr=weak_ptr<T>;
}
#endif //MEMORY_SHORTHAND
#endif //_MEMORY_

#ifndef NUMBER_SHORTHAND
#define NUMBER_SHORTHAND
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned long long ulonglong;
typedef unsigned int const uintc;
typedef unsigned long const ulongc;
typedef unsigned short const ushortc;
typedef unsigned char const ucharc;
typedef unsigned long long const ulonglongc;
typedef signed int sint;
typedef signed long slong;
typedef signed short sshort;
typedef signed char schar;
typedef signed long long slonglong;
typedef signed int const sintc;
typedef signed long const slongc;
typedef signed short const sshortc;
typedef signed char const scharc;
typedef signed long long const slonglongc;

typedef unsigned int* uintp;
typedef unsigned long* ulongp;
typedef unsigned short* ushortp;
typedef unsigned char* ucharp;
typedef unsigned long long* ulonglongp;
typedef unsigned int const* uintcp;
typedef unsigned long const* ulongcp;
typedef unsigned short const* ushortcp;
typedef unsigned char const* ucharcp;
typedef unsigned long long const* ulonglongcp;
typedef signed int* sintp;
typedef signed long* slongp;
typedef signed short* sshortp;
typedef signed char* scharp;
typedef signed long long* slonglongp;
typedef signed int const* sintcp;
typedef signed long const* slongcp;
typedef signed short const* sshortcp;
typedef signed char const* scharcp;
typedef signed long long const* slonglongcp;

typedef unsigned int* uintpc;
typedef unsigned long* ulongpc;
typedef unsigned short* ushortpc;
typedef unsigned char* ucharpc;
typedef unsigned long long* ulonglongpc;
typedef unsigned int const* uintcpc;
typedef unsigned long const* ulongcpc;
typedef unsigned short const* ushortcpc;
typedef unsigned char const* ucharcpc;
typedef unsigned long long const* ulonglongcpc;
typedef signed int* sintpc;
typedef signed long* slongpc;
typedef signed short* sshortpc;
typedef signed char* scharpc;
typedef signed long long* slonglongpc;
typedef signed int const* sintcpc;
typedef signed long const* slongcpc;
typedef signed short const* sshortcpc;
typedef signed char const* scharcpc;
typedef signed long long const* slonglongcpc;
#endif //signed_SHORTHAND

#ifndef CAST_SHORTHAND
#define CAST_SHORTHAND
#define scast static_cast
#define dcast dynamic_cast
#define rcast reinterpret_cast
#define ccast const_cast
#endif //CAST_SHORTHAND