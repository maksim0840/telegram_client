#ifndef GI_HPP
#define GI_HPP

#define GI_VERSION_MAJAOR (2)
#define GI_VERSION_MINOR (0)
#define GI_VERSION_MICRO (0)

#ifdef GI_INLINE
#define GI_INLINE_DECL inline
#else
#define GI_INLINE_DECL
#endif

// typically clang might warn but gcc might complain about pragma clang ...
#ifdef GI_CLASS_IMPL_PRAGMA
#ifndef GI_CLASS_IMPL_BEGIN
#define GI_CLASS_IMPL_BEGIN \
  _Pragma("GCC diagnostic push") \
      _Pragma("GCC diagnostic ignored \"-Woverloaded-virtual\"")
#endif

#ifndef GI_CLASS_IMPL_END
#define GI_CLASS_IMPL_END _Pragma("GCC diagnostic pop")
#endif
#else
#define GI_CLASS_IMPL_BEGIN
#define GI_CLASS_IMPL_END
#endif

#include "base.hpp"
#include "container.hpp"
#include "enumflag.hpp"
#include "exception.hpp"
#include "expected.hpp"
#include "object.hpp"
#include "objectclass.hpp"
#include "string.hpp"

// check that include path has been setup properly to include override
#if defined(__has_include)
#if !__has_include(<glib/glib_extra_def.hpp>)
#warning "overrides not found in include path"
#endif
#endif

#endif // GI_HPP
