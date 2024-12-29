#ifndef _GI_GTK_GTK_EXTRA_DEF_HPP_
#define _GI_GTK_GTK_EXTRA_DEF_HPP_

namespace gi
{
namespace repository
{
namespace Gtk
{
namespace base
{
#ifdef _GI_GTK_LIST_STORE_EXTRA_DEF_HPP_
// deprecated (as of about 4.9.1)
// so only define implementation here if declaration has been included
template<typename... Args>
Gtk::ListStore
ListStoreExtra::new_type_() noexcept
{
  GType columns[] = {traits::gtype<Args>::get_type()...};
  return ListStoreBase::new_(G_N_ELEMENTS(columns), columns);
};
#endif
} // namespace base

} // namespace Gtk

} // namespace repository

} // namespace gi

#endif
