// no code generation is needed
// all required functionality is required by the basic code
#include <gi/gi.hpp>

#include <iostream>

namespace GObject_ = gi::repository::GObject;

// class must be a ObjectImpl to support properties and signals
class Person : public GObject_::impl::ObjectImpl
{
private:
  // the implementation/definition part of the properties
  // (could also be constructed here, but let's do that in constructor below)
  // this provides interface to set/get the actual value
  gi::property<int> prop_age_;
  gi::property<std::string> prop_firstname_;
  gi::property<std::string> prop_lastname_;

public:
  // likewise for signal
  // public because there is no extra interface for owning class
  // (both owner and outside can connect and/or emit)
  // btw, using Person in this signature would not be the way to go,
  // should stick to a plain wrapped type
  gi::signal<void(GObject_::Object, int)> signal_trigger;
  // std::string also works here,
  // but that would cost an extra allocation during invocation
  gi::signal<void(GObject_::Object, gi::cstring_v)> signal_example;

public:
  Person()
      : ObjectImpl(this),
        // the setup parameters shadow the corresponding g_param_spec_xxx
        // so in practice define the property (name, nick, description)
        // along with min, max, default and so (where applicable)
        prop_age_(this, "age", "age", "age", 0, 100, 10),
        prop_firstname_(this, "firstname", "firstname", "firstname", ""),
        prop_lastname_(this, "lastname", "lastname", "lastname", ""),
        signal_trigger(this, "trigger"), signal_example(this, "example")
  {}

  // the public counterpart providing the same interface
  // as with any wrapped object's predefined properties
  gi::property_proxy<int> prop_age() { return prop_age_.get_proxy(); }

  gi::property_proxy<std::string> prop_firstname()
  {
    return prop_firstname_.get_proxy();
  }

  gi::property_proxy<std::string> prop_lastname()
  {
    return prop_lastname_.get_proxy();
  }

  void action(int id)
  {
    std::cout << "Changing the properties of 'p'" << std::endl;
    prop_firstname_ = "John";
    prop_lastname_ = "Doe";
    prop_age_ = 43;
    std::cout << "Done changing the properties of 'p'" << std::endl;
    // we were triggered after all
    signal_trigger.emit(id);
  }
};

void on_firstname_changed(GObject_::Object, GObject_::ParamSpec)
{
  std::cout << "- firstname changed!" << std::endl;
}
void on_lastname_changed(GObject_::Object, GObject_::ParamSpec)
{
  std::cout << "- lastname changed!" << std::endl;
}
void on_age_changed(GObject_::Object, GObject_::ParamSpec)
{
  std::cout << "- age changed!" << std::endl;
}

int
main(int /*argc*/, char ** /*argv*/)
{
  Person p;
  // register some handlers that will be called when the values of the
  // specified parameters are changed
  p.prop_firstname().signal_notify().connect(&on_firstname_changed);
  p.prop_lastname().signal_notify().connect(on_lastname_changed);
  p.prop_age().signal_notify().connect(&on_age_changed);

  // now change the properties and see that the handlers get called
  p.action(0);

  // (derived) object can be constructed on stack for simple cases
  // but in other (real) cases it is recommended that it is heap based.
  // so as not to have a naked ptr, it can be managed by a (special) shared
  // ptr that uses the GObject refcount for (shared) ownership tracking
  auto dp = gi::make_ref<Person>();
  // however, the GObject world has no knowledge of the subclass
  // (each instance is 1-to-1 with Person instance though).
  // so when we get something from that world, we can (sort-of dynamic)
  // cast to the subclass
  auto l = [](GObject_::Object ob, int id) {
    std::cout << " - triggered id " << id << std::endl;
    // obtain Person
    auto lp = gi::ref_ptr_cast<Person>(ob);
    if (lp)
      std::cout << " - it was a person!" << std::endl;
    // it really should be ...
    assert(lp);
  };
  dp->signal_trigger.connect(l);
  dp->action(1);

  return 0;
}
