cppgir(1) - GObject-Introspection C++ binding wrapper generator
==================================================================


## SYNOPSIS

`cppgir` [OPTION...] `--output` _DIRECTORY_ GIR...


## DESCRIPTION

`cppgir` reads each of the specified GIR and converts these (and any
dependencies) into C++14 wrapper code that collectively then make up a
'binding' (in
[GObject-Introspection](https://wiki.gnome.org/Projects/GObjectIntrospection)
terminology). Each GIR can be specified as a full pathname to the `.gir` file
or simply by the basename (i.e. no path or `.gir` suffix), with or without
version. Of course, in the latter case, the `.gir` must be in a standard
location, or other options must specify additional whereabouts.

## OPTIONS

See [BACKGROUND](#background) later on for further details on some of the
concepts used in the following descriptions.

* `--output` _DIRECTORY_:
  Specifies the top-level directory in which to generate code.
  It will be created if it does not yet exist.

* `--gir-path` _PATHS_:
  Adds a colon-separated list of additional directories within which
  to (recursively) search for a `.gir` file (if not specified by full pathname).

* `--debug` _LEVEL_:
  Debug level or level of verbosity, higher numbers are more verbose.

* `--ignore` _FILES_:
  Adds a colon-separated list of so-called ignore files.

* `--suppression` _FILES_:
  Adds a colon-separated list of so-called suppression files.

* `--gen-suppression` _FILE_:
  Specifies a suppression file to generate during this run.

* `--class`:
  Requests generation of implementation class code needed for subclassing.

* `--class-full`:
  Requests generation of a plain as-is C signature fall-back method for
  an otherwise unsupported unwrapped method.  Only applicable if
  `--class` is also specified.  It also requires use of the latest custom
  subclass (signature) approach (see below for details on that),
  as these plain methods are not "activated" in case of legacy approach
  (for backwards compatibility).

* `--expected`:
  Use an error return type based on [std::expected](http://wg21.link/p0323) proposal
  (as opposed to throwing exception).

* `--dl`:
  Use dlopen/dlsym to generate (most) calls rather than usual "direct" calls.
  As such, a great many calls might then fail at runtime.  So, if combined
  with `--expected` all those calls will use the above error return type.

* `--dump-ignore`:
  (only if compiled with embedded ignore) Dumps embedded ignore data.

## ENVIRONMENT

In stead of command-line options, environment variables can also be used. Note,
however, that options are still taken into account even when variables have
been set. The following environment variables are considered, and have the same
meaning as the corresponding command-line option:

    `GI_DEBUG`, `GI_IGNORE`, `GI_SUPPRESSION`, `GI_GEN_SUPPRESSION`, `GI_OUTPUT`,
    `GI_CLASS`, `GI_CLASS_FULL`, `GI_EXPECTED`, `GI_DL`, `GI_GIR_PATH`

In addition to the above, `GI_GIR` can specify a colon-separated lists of GIRs
(specified as on command-line).  `XDG_DATA_DIRS` is also used as additional
source of directories to search for GIRs (within a `gir-1.0` subdirectory).


## BACKGROUND

### API v2

Note that v2 API is somewhat different than previous API, so some porting of
existing code may be needed.  See also later section for a rationale and
discussion on changes.

The generated code provides a straight binding as specified by the annotations,
so everything is pretty much where expected, such as methods within classes
in turn within namespaces.  For example, all `GObject` types are within
namespace `gi::repository::GObject`.  With that in mind, it should be easy
to use and navigate in generated code, along with following comments:

* As customary, anything within a `detail` or `internal` namespace is not meant
  for public use and subject to change.  The top-level gi namespace defines
  a few things that make up public API which is meant to be stable
  (though at this stage of maturity no full guarantee is provided).

* Some generated code may have `_` (underscore) appended to it simply to avoid
  clashing with a reserved keyword (or a preprocessor definition).  It has
  no special (reserved) meaning otherwise.

* However, anything with leading underscore (if encountered) should be considered
  as internal (and not meant for public API).

In overall, the generated code is very lightweight and clear, easily understood
and with little runtime overhead, as also illustrated by the following
overview of wrappers for various kinds of types.  Note that almost all of
them essentially wrap a pointer and therefore should be checked for validity
prior to many uses as with any "smart pointer"
(e.g. using provided `operator bool()`).

**Objects.**
A GObject is a single pointer along with class code that manages a single
refcount (including decrement upon destruction).  The refcount it manages is
either received/taken from a `full` transfer, or `ref_sink`'ed (in case of
`none`/`floating` transfer, see also discussion in subsequent section on the
intricacies of the latter and theoretical edge cases).

**Boxed Types.**
Similarly, but with a minor twist, wrappers for a boxed GType `MyBox` come in 2
kinds; an owning `MyBox` and a non-owning `MyBox_Ref`.  In both cases, the
wrapper is again a single pointer with some suitable/applicable helper methods.
The former essentially acts a "unique ptr" (with `g_boxed_free` deleter) whereas
the latter acts as a "naked ptr/reference" (without any ownership or cleanup).
Obviously, for the latter case, all the usual caution regarding dangling
references (etc) applies. The latter are used for transfer `none` cases and
the former in transfer `full` situations.  In case a safe "reference" needs to
be kept around (e.g. in some member), then a `_Ref` can be `.copy_()`'d (which
uses `g_boxed_copy`) to an owning wrapper. The above semantics also imply that
the owning wrapper is move-only (and again `.copy_()` yields a copy).  However,
there are quite some cases where a boxed copy is based on a refcount (which also
preserves the box identity/pointer). Those cases have been specially marked (in
overrides) to make the owning wrappers copyable as well.  Likewise, a `_Ref` of
such cases can be (implicitly) assigned/copied to an owning one (in each case
triggering a `g_boxed_copy` which is then known to be plain and cheap).
If desired, additional wrappers could be marked as copyable, in which case a
wrapper copy invokes a potentially more expensive (and non-identity preserving)
`g_boxed_copy`.  Also, or alternatively, if `GI_ENABLE_BOXED_COPY_ALL` is
defined and truthy, then all boxed wrappers are copyable in that way.

**Record Types.**
Plain records (i.e. structs with no registered GType) are handled in a similar
fashion, with `g_free` as "deleter" (and without any copy support).  Since no
lifecycle resource management (construction, destruction) is available for such
types, there are (quite some) limitations to what code generation or binding can
do here (see also discussion in corresponding section).

**Strings.**
A string (e.g. `char*`) is also regarded and wrapped in a similar way.  That is,
a `gi::cstring` wraps (and owns and manages) a C `char*` and `gi::cstring_v` is
the corresponding non-owning variant.  Obviously, the former bears resemblance
to `std::string` whereas the latter to `std::string_view`. In fact, as there is
no real definitive "string API" (in C or glib), their API is fairly similar
(though not guaranteed identical) to the `std` counterparts. Also, various
conversions from/to `std` counterparts should allow for convenient type
interchange.  Additional integration with other string types is also possible by
further specialization of `gi::convert::converter` (see `gi/string.hpp` source
for details).

**Collections**.
That is, `GList`, `GSList`, `GPtrArray`, `GHashTable` or plain arrays
(zero-terminated or not).  Similar to `std` container, each collection wrapper
is a templatized `gi::Collection` type, with (a.o.) a type parameter for the
contained type.  As with some of the above types, such wrappers come in an
owning and non-owning variants, as specified by another (type) parameter and
obtained from annotations, i.e. transfer `none`, transfer `container` or transfer
`full`.  Note that the "ownership" specifies both ownership of the container and
of the contained elements. Of course, where needed, code generation will select
and specify the proper type (e.g. as function parameter).  Following aspects
are worth mentioning;

* Templatized constructors and conversion operators support construction
  from/of and assignment from/to (e.g.) `std` container types.  Likewise
  so for "similar" (duck-ed) types, where "similar" refers to member types and
  constructor signatures.

* A (`std`) container-ish API is also provided, though neither identical
  nor fully compatible (a.o. due to limitations of the C wrappee's API).
  However, the `none` (ownership) variant is considered read-only and so
  it does not provide any "modification" API parts and only a `const` iterator.
  As almost no wrapper methods are `const`, an `auto p : coll` (range-for)
  pattern is recommended (wrappers are cheaply copied).  Other variants do
  support modification as well as iteration that allows for a `auto &p : coll`
  pattern (if so desired).  In particular, this applies to the `full` variant,
  which is the recommended one for "standalone" use (as container), as it
  safely manages ownership of both itself and elements.

* Wrappers of refcounted collections (`GPtrArray`, `GHashTable`) are
  otherwise similar to object wrappers.  So they *always* manage a refcount (and
  are copyable) regardless of ownership variant (none, etc). The other wrappers
  are similar to boxed wrappers, e.g. copyable in `none` variant, but otherwise
  assume unique ownership and are non-copyable.

* A `gi::CollectionParameter` may also used by code generation for a function
  input parameter.  In case of `none` ownership, this type/instance will
  temporarily hold ownership of a collection that may be created by conversion
  from another container.  Temporarily here refers to the duration of the call
  during which the parameter instance exists.  It is not (and should not be)
  used elsewhere.

In short, one can choose to work with `std` types and convert to
collection wrappers upon function call/return, but for simple cases (or beyond),
the collection wrapper might well serve (without conversion).

**Plain Types.**
Various enum, (static) method, functions, typedef (for callback) fill in the
rest.

**Functions.**
Functions that involve the usual `GError` return pattern are wrapped in a few
ways.  On the one hand, in a straight way, where the error is a (wrapped error)
output parameter.  Alternatively, the error parameter is removed from the
signature.  In that case it is "returned" by either throwing the (wrapped) error
(which is also a `std::exception` subclasss), or by returning a suitable
`expected` type (with the wrapped error type as error type). While throwing is
default behaviour, the latter can be requested using `--expected` option.

In case of a `GError` in (function) callback or virtual method signature, it is
always retained as a (wrapped) error output parameter and preferably used to
report an error that way.  Alternatively, an exception can be thrown, preferably
then a `GLib::Error` instance.  Callback wrapping code will catch any exception
and report (to `C` caller) using `GError` output along with a zero-initialized
return value, which is likely but not necessarily a good choice.

Note, however, that the aforementioned `catch` only applies if exception support
is enabled.  Auto-detection of this should usually work, but if needed can be
specified by defining `GI_CONFIG_EXCEPTIONS` expclitly (truth/falsy).

**Subclasses and Interfaces.**
Some additional specifications on how subclasses and interfaces are mapped
may also be in order. A subclass in the GObject world is directly mapped as a
subclass in the C++ binding. However, if a GObject implements an interface, the
generated class does not inherit from the interface's (generated) class. This
is mostly of a matter of implementation choice (and to ensure its lightweight
simplicity). However, knowledge of implemented interfaces is not always
available at compile time, e.g. in case of dynamically loaded GStreamer
elements (though it is more likely in case of Gtk hierarchy). Since there would
be no inheritance in the dynamic case, a consistent choice is not to have it at
any time. However, for ease of use, some helper code is generated when an
implemented interface is known at generation/compile time, as illustrated in
the following snippet from an example

```c++
    // use a cast if not known, either to a class or interface
    auto bin = gi::object_cast<Gst::Bin>(playbin_);
    // known at compile time; overloaded interface_ method
    auto cp = bin.interface_ (gi::interface_tag<Gst::ChildProxy>());
```


### SUBCLASS IMPLEMENTATION API

There may be times when one would want to make a custom subclass of GObject, or
of some Gtk widget. In the same vein, (current) implementation choices imply
that one should not simply inherit from `Gtk::Window`. Part of the motivation
here is that such subclassing depends on style and setting, i.e. it is rather
rare when in a GStreamer setting, but less so in e.g. Gtk. As such, the
possibly rare cases should not burden or complicate the basic wrapping usecase.

So, how to subclass then? By a slight twist by using the `impl` namespace
variations, as in following excerpt from an example:

```c++
class TreeViewFilterWindow : public Gtk::impl::WindowImpl
{
  // ...
public:
  // Assume (hypothetically) that Window also implements FakeInterface
  // with a set_focus method, then a compilation failure will be triggered (as
  // it can no longer be detected whether set_focus is defined in this class).
  // Then the following inner struct is needed to resolve so manually;
  struct DefinitionData
  {
    // the last parameter specifies whether the method is defined
    // (which may well be false in all class/interface cases if not defined)
    GI_DEFINES_MEMBER(WindowClassDef, set_focus, true)
    GI_DEFINES_MEMBER(FakeInterfaceDef, set_focus, false)
  };
  // NOTE for the auto-detection to work, the methods must be accessible
  // so either they should be defined public, or (e.g.) WindowClassDef
  // must be declared friend, or the above manual resolution can be used.

TreeViewFilterWindow () : Gtk::impl::WindowImpl (this)
  {
    // ...
  }

  void set_focus_ (Gtk::Widget focus) noexcept override
  {
  }
};
```
Parent (class or interface) methods can then be overridden or implemented
in the usual way by simply defining them in the subclass.  It is also possible
to define custom signal and properties in the subclass, as illustrated in the
`gobject.cpp` example.  As mentioned, the inner `DefinitionData` struct in the
above fragment is usually not needed, but only in case of conflict/duplication
of class/interface member(s).

Since this is considered an optional feature, the `impl` parts are not generated
by default, but only if the `--class` option is specified.  Since the virtual
methods share some similarities with callbacks they are also subject to some
limitations (see corresponding section).  As such, it may happen that some
virtual methods do not have a wrapper.  If the `--class-full` option is
specified, then a passthrough virtual method (with C signature as-is) is then
generated instead, which can then be overridden and implemented as a fallback.
So the custom type registration (that happens behind the scenes) can then still
be used, albeit at the expense of dealing with a plain C signature and types
(which is similar to directly calling a C function as a fallback if no wrapper
function was generated for some reason).


### CODE LAYOUT AND BUILD SETUP

The generated code is written to the top-level with the following layout.
Each GIR namespace has a corresponding subdirectory, say `ns`
(and also a C++ namespace, `cppgir::repository::ns`).  The top-levels
headers for a namespace are then:

* `ns.hpp`:
  a regular header providing the namespace's declarations.
  It will also include the dependent namespaces' top headers.
  If the macro `GI_INLINE` is defined, then it will also include ...
* `ns_impl.hpp`:
   contains the definitions corresponding to the declarations.
  Normally, this would be a `.cpp` file, but as they might be included directly
  in the inline case, they have been named `xxx_impl.hpp` instead.
* `ns.cpp`:
  this merely includes `ns_impl.hpp` and is as such no different
  than the latter, except for more traditional naming.
  Compiling this file in the non-inline case provides all the definitions
  for the namespace in the resulting object file.

So, in summary, it comes down to setting up the build system to build each of
the namespaces' `.cpp`, as is also done in this repo's CMake build setup.
There is one other shortcut build setup that is illustrated by the `gtk-obj.cpp`
example file, which includes all definitions (recursively):

```c++
#define GI_INCLUDE_IMPL 1
#include <gtk/gtk.hpp>
```

Note, however, this is only possible if there is exactly 1 top-level namespace,
as doing this for several namespaces will lead to duplicate definitions.

Some items (functions, types) may be marked as deprecated (in source code).
while still present in GIR data.  Wrappers will still be generated and
`pragma` are issued to avoid warnings that might otherwise occur.
Generic `gi` support tries to avoid using deprecated code.  There is, however,
one exception regarding the use of `g_object_newv`, which is deprecated
but may have to be used if support for an older GLib is required.
This can be arranged by defining `GI_OBJECT_NEWV` (and the deprecation
warning should also be silenced when dealing with newer version).
If the items are also marked deprecated in GIR data, then these are skipped
by default.  However, if the string `deprecated:<NAMESPACE>:<VERSION>`
matches (a regexp) in specified ignore data/files, then deprecated items
will be considered for the namespace in question, after being checked as
usual against the ignore list.

If you have specified the `--class` option, then the generated code will
possibly contain classes that inherit from several classes (representing
interfaces). Since various interfaces may have overlapping member names, this
might trigger compilation warnings. These are not suppressed by default, as you
may need to be made aware of this. However, if it does no harm in your
particular case, then defining `GI_CLASS_IMPL_PRAGMA` should arrange for proper
suppression.


### OVERRIDING OR EXTENDING

It is possible to add functions or methods or override existing names (by
effect of name hiding). To this end, the generated code contains various
'optional include hooks' using the `__has_include` directive. This way, code in
externally supplied (include) files can be inserted into the class definition
chain. There are roughly 3 such 'hook points':

* **initial setup**:
this part is (conditionally) included before the namespace's C headers are included.
This allows specifying define's to tweak subsequent headers or to add
headers that also need to be include'd, and which may not have been specified
in the GIR.

* **class definition**:
these hooks allow extending the wrapped class with new or tweaked methods

* **global extra definitions**:
these are included after all generated code, and supports adding of new global
functions, typedef's, type trait helper declarations, ...

The reader is invited to examine the default overrides in this repo as well as
the generated code to see how this fits together based on a simple naming
scheme and use of macros. In particular, see the provided `GLib` overrides.
Suffice it to add that the `_def` suffix refers to 'default' as supplied by
this repo and which are installed alongside the common headers. The
corresponding non-suffixed filenames should be used by project specific custom
additions.


### CODE GENERATION

It might be necessary to exclude a GIR entry from processing, either because it
is a basic type handled by custom code (e.g. `GObject`, `GValue`, ...) or
because of a faulty annotation. The latter can be a glitch in the annotation
itself, or one that actually refers to a symbol in a non-included private
header. The exclusion can be directed by so-called ignore files, and at least
one such is supplied as a system default ignore containing known and essential
cases to exclude (and without which code generation would not produce valid
code). Such a file consists of lines of regular expressions (`#` commented
lines are ignored). At generation time, each symbol is turned into a
`<NAMESPACE>:<SYMBOLKIND>:<SYMBOL>` string, and excluded if it matches one of
the lines' regular expression. So, for instance, `GObject:record:Value`
prevents processing of `GValue`, since there is already special-case code for
that in the common header code. Further expression examples are found in the
default ignore file. Additional files can be specified by the `--ignore`
option.

As each entry is processed, some notification may be given regarding a
perceived inconsistency in an annotation or an unsupported case (see also [BUGS
AND LIMITATIONS](#bugs-and-limitations)). When the reported cases have been
(manually) checked and considered harmless, the corresponding notices can be
suppressed by specifying suppression files to `--suppression`. The format of
such files is the same as ignore files, except that a match then simply serves
to decrease reporting verbosity. Such a file could be hand-crafted, but it can
also be auto-generated by a run when specifying `--gen-suppression`.

Besides excluding problematic GIR parts, one might also consider solutions to
some problematic GIRs used by other projects, such as fixed GIRs maintained by
[gtk-rs](https://gtk-rs.org/gir/book/tutorial/finding_gir_files.html#gtk-dependencies)
in the referenced [repo](https://github.com/gtk-rs/gir-files).

### (RATIONALE OF) v2 CHANGES

Consider the following python session using gobject-introspection:

    >>> import gi
    >>> gi.require_version('Gst', '1.0')
    >>> from gi.repository import Gst
    >>> Gst.init(None)
    >>> c = Gst.caps_from_string('video/x-raw')
    >>> c.get_structure(0)
    <Gst.Structure object at 0x7fe284096760 (GstStructure at 0x1bb4420)>
    >>> c.get_structure(0)
    <Gst.Structure object at 0x7fe2840b5d00 (GstStructure at 0x1bb43a0)>

What happens here?  A different `GstStructure*` is created each time, even
though the same one is returned (by C code) in each case.  The python binding
here has no other choice than to use `g_boxed_copy()` on the transfer `none`
return value.  If it would not, it would be carrying around an unguarded/unowned
and hence potentially dangling pointer (in some `PyObject` wrapper), which is
a definite no-go in a scripted setting that must always ensure valid objects.

v1 API followed a similary "scripted" style approach where all objects/pointers
should always be safe and valid, with (roughly) `std::shared_ptr` in place of
`PyObject`.  Of course, also then with similar (copy) effects as in the above
excerpt and in e.g. [issue #32](https://gitlab.com/mnauw/cppgir/-/issues/32).

v2 now follows a different approach.  After all, C++ is much closer to C, and it
is customary to mind about (potentially dangling) references and such, and where
and how (not) to use e.g. `std::string_view`.  And so while types/objects are
now no longer always "owning" (and as such always safe), the type conventions do
clearly specify whether or not they do (own).  As such, standard C++ practices
should handle what v2 API provides, while avoiding superfluous and potentially
surprising copies or any other "automagic".  In particular, the v2 bindings
are therefore even more "tight and direct" than before, with a typical wrapper
being only a cast away from the wrappee (and matching in size and semantics).

**Migration.**
In practice, only limited changes have been needed in the included examples.
Of course, your mileage may vary, depending on usage of "boxed types" as well
as use of (type deduction) `auto` versus explicit type specification.
Some `_Ref` types may have to be used instead here or there, as well as possibly
some `std::move` on "owning" variants (unless overall boxed copy is enabled).
For reasons of consistency and to avoid collision with generated methods,
some more "custom methods" may have had `_` appended
(e.g. `CBoxed::allocate_()`).


## BUGS AND LIMITATIONS

The generated code's coverage is pretty good and comfortably serves most
cases that arise in practice as also illustrated by the examples.
Nevertheless, the following should be mentioned:

**Callback types.**
Only callback types that have an explicit `user_data`
parameter are supported. That includes (fortunately) cases such as connecting
to a signal, or a `GstPadProbeCallback`, though a `GstPadChainFunction` is
excluded. The reason is a technical one; the `user_data` parameter is used to
pass data used by callback wrapper code. A typical (script) runtime binding
handles this using [libffi](https://github.com/libffi/libffi)'s closure API. In
effect, a little bit of executable code is then generated at runtime, and the
address of that code then essentially serves as surrogate `user_data` that can
carry extra meta-data for use by the runtime. This could also be employed here
to lift the `user_data` limitation, it would take a bit extra work, but would
more importantly then also incur an additional dependency.

**Callback handling.**
Even if `user_data` is present, other aspects of a callback signature
may not be supported (at this time), e.g. certain (sized) array parameters.
However, few (if any) of such actual cases are known at this time.
Note that both signals and virtual methods are somewhat similar to a callback
and as such share similar limitations.

Whereas the above items could (in theory) be resolved, the following are more
inherent limitations (by the very context and nature of e.g. annotations).
Fortunately, though, the practical impact is fairly limited (if any).

**const handling.** In C++, this is a Bigger Thing. For instance, a simple
'getter' should preferably be marked const. However, on the original C-side of
things, only very limited consideration is given to this. Even if there is some
`const`, it is not treated with all that much respect, e.g.
`g_value_take_boxed` starts `const` but it is merrily cast away along the way.
As such, there is not much to find on const-ness in annotation data, and so no
point in inventing any. Rather, the focus is simply on getting the proper
function calls done along with automagic refcount and resource management (much
as any runtime binding would do, with no regard for const whatsoever in that
case).

In particular, methods are usually not marked `const` either, as there is
similarly no (semantic) data to decide either way. As such, it is not
recommended to use `const` wrapper types. However, they may arise due to generic
templates or when captured in a lambda. In such cases, the helper (template)
type `gi::cs_ptr` may be useful, or alternatively one might set the option to
mark all code-generated methods as `const`.

**Floating (into darkness).**
[Gobject docs](https://docs.gtk.org/gobject/floating-refs.html) mention the
following about floating references (i.e. transfer `floating`);

> Floating references are a C convenience API and should not be used in modern
GObject code. Language bindings in particular find the concept highly
problematic, as floating references are not identifiable through annotations,
...

Indeed, by the time `floating` makes it into the parsed annotation, it has
become `none`.  And in case of a "factory" `some_widget_new()`, `floating`
behaves more like `full` as the caller must "take ownership" to avoid a leak. So
a "floating" `none` is quite different from a "real" `none` (e.g. "getter"
method).  But no way to know from annotation data.  So, in case of `none`, an
object wrapper always `ref_sink()`s.  If it was floating, it has taken suitable
ownership.  If it was really none, then it is now managing an extra refcount.
And in either case, it will release/decrement upon destruction.  Essentially,
this follows the recommendation given in referenced docs.  In practice, it
actually Just Works.

It gets really tricky when this is combined with e.g. lists. So what does `none`
mean in this case (in annotation)?  In the worst case, the contained elements
might actually be floating, so one would have to go through the list and
`ref_sink` them all (un)conditionally?  Suffice it to say, no such "automagic"
is handled/injected by any wrapper code.  Fortunately, at this time there does
not seem to be such a "multiple factory" API. Even if there were, then in
practice the calling code is likely to loop over the list and access the
elements. The ensuing C++ wrappers (even if existing only briefly) would then
effectively `ref_sink()`, so again we are ok.  And last but not least, by the
above quoted recommendation, there should be no such new tricky API coming
along.  So, again, it Just Works.  If needed, any such old or new API can and
should be handled by custom overrides.

**Boxed (by darkness).**
This refers to so-called "plain records" which are "C structs" with no
registered `GType` (referred to as "C boxed" types in `cppgir` code), e.g.
`GOptionEntry` or `GstMapInfo`.  While their fields may be described in
annotations, there is no information regarding the "ownership" of any data
(which may even vary upon context).  In particular, also no way to create/free.
This corresponds with their frequent stack-allocated use in C code in typically
"low-level" API which is usually not considered "binding friendly".  Based on
the mild assumption that 0-initialized data makes a valid instance, they are
treated somewhat similar to (GType) boxed types and as such can be used in some
limited (function call) situations.  Any improvement beyond that is likely to
remain in the purview of overrides.


### WORKAROUNDS

As C++ allows direct mixing/calls with C, there are usually some fallback
workarounds when confronted with one of the limitations.  First of all,
note that a C++ wrapper typically has e.g. a `gobj_()` method that
provides the underlying C pointer/object.  Conversely, `gi::wrap` can be
used to obtain a wrapper from a C pointer/object obtained by some means.
With that in mind, the following are some workarounds;

* function call;
using/given the above, the C function can then (simply) be called directly

* custom subclass virtual method;
use `--class-full` to generate a virtual method with plain C signature

* signal;
use `Object::connect_unchecked` (see also `gst.cpp` example)

* callback;
use `gi::callback_wrapper` (see also in same example location as above)


## SEE ALSO
g-ir-scanner(1)
