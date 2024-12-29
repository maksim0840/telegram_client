#ifndef _GI_GLIB_SETUP_DEF_HPP_
#define _GI_GLIB_SETUP_DEF_HPP_

// missing in GIR
// recent version re'define functions to system functions,
// but may not include suitable headers, so force fallback to wrapping
// in recent version, it may also be included by glib-unix below
// so do so before including that one
#ifdef G_OS_UNIX
#define G_STDIO_WRAP_ON_UNIX 1
#endif
#include <glib/gstdio.h>

#ifdef G_OS_UNIX
// missing in GIR
#include <glib-unix.h>
#endif

namespace gi
{
namespace repository
{
// enable various ref-based boxed copy
GI_ENABLE_BOXED_COPY(GByteArray)
GI_ENABLE_BOXED_COPY(GBytes)
GI_ENABLE_BOXED_COPY(GRegex)
GI_ENABLE_BOXED_COPY(GMatchInfo)
GI_ENABLE_BOXED_COPY(GVariantBuilder)
GI_ENABLE_BOXED_COPY(GVariantDict)
GI_ENABLE_BOXED_COPY(GDateTime)
GI_ENABLE_BOXED_COPY(GTimeZone)
GI_ENABLE_BOXED_COPY(GKeyFile)
GI_ENABLE_BOXED_COPY(GMappedFile)
GI_ENABLE_BOXED_COPY(GMainLoop)
GI_ENABLE_BOXED_COPY(GMainContext)
GI_ENABLE_BOXED_COPY(GSource)
GI_ENABLE_BOXED_COPY(GMarkupParseContext)
GI_ENABLE_BOXED_COPY(GThread)
GI_ENABLE_BOXED_COPY(GOptionGroup)

} // namespace repository
} // namespace gi

#endif // _GI_GLIB_SETUP_DEF_HPP_
