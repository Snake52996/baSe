#ifndef CxKANOAXDP_standard_fix_H_
#define CxKANOAXDP_standard_fix_H_
#ifndef __has_c_attribute
#define __has_c_attribute(x) 0
#endif
#if __has_c_attribute(maybe_unused)
#define Unused [[maybe_unused]]
#else
#define Unused
#endif
#if __has_c_attribute(nodiscard)
#define NoDiscard [[nodiscard]]
#else
#define NoDiscard
#endif
#endif