#ifndef GEEVM_COMMON_MEMORY_H
#define GEEVM_COMMON_MEMORY_H

#if defined(__has_feature)
#if __has_feature(address_sanitizer) && !defined(__SANITIZE_ADDRESS__)
#define __SANITIZE_ADDRESS__
#endif
#endif

#if defined(__SANITIZE_ADDRESS__)
#include <sanitizer/asan_interface.h>
#define ASAN_POISON_MEMORY_REGION(addr, size) __asan_poison_memory_region((addr), (size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) __asan_unpoison_memory_region((addr), (size))
#else
#define ASAN_POISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#endif

#endif // GEEVM_COMMON_MEMORY_H
