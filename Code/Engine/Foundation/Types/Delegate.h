#pragma once

#include <Foundation/Basics.h>

/// \brief Base class for plDelegate
class plDelegateBase
{
public:
  union InstancePtr
  {
    void* m_Ptr;
    const void* m_ConstPtr;
  };

  PL_ALWAYS_INLINE plDelegateBase() { m_Instance.m_Ptr = nullptr; }

protected:
  InstancePtr m_Instance;
};

/// \brief A generic delegate class which supports static functions and member functions.
///
/// A delegate is a function pointer that may be used to call both simple C functions, as well
/// as member functions of some class, which requires a 'this' pointer to go along with.
/// The delegate can capture both these use cases dynamically, so this distinction does not need
/// to be made statically, but is the choice of the user who assigns some function to the delegate.
///
/// Delegates have a rather strange syntax:
///
/// \code{.cpp}
///   using SomeCallback = plDelegate<void (plUInt32, float)>;
/// \endcode
///
/// This defines a type 'SomeCallback' that can call any function that returns void and
/// takes two parameters, the first being plUInt32, the second being a float parameter.
/// Now you can use SomeCallback just like any other function pointer type.
///
/// Assigning a C function as the value to the delegate is straight-forward:
///
/// \code{.cpp}
///   void SomeFunction(plUInt32 i, float f);
///   SomeCallback callback = SomeFunction;
/// \endcode
///
/// Assigning a member function to the delegate is a bit more complex:
///
/// \code{.cpp}
///   class SomeClass {
///     void SomeFunction(plUInt32 i, float f);
///   };
///   SomeClass instance;
///   SomeCallback callback = plDelegate<void (plUInt32, float)>(&SomeClass::SomeFunction, &instance);
/// \endcode
///
/// Here you have to construct a delegate of the proper type and pass along both the member function pointer
/// as well as the class instance to call the function on.
///
/// Using the delegate to call a function is straight forward again:
///
/// \code{.cpp}
///   SomeCallback callback = ...;
///   if (callback.IsValid())
///     callback(4, 2.0f);
/// \endcode
///
/// Just treat the delegate like a simple C function and call it. Internally it will dispatch the call
/// to whatever function is bound to it, independent of whether it is a regular C function or a member function.
///
/// The check to 'IsValid' is only required when the delegate might have a nullptr bound (i.e. no function has
/// been bound to it). When you know the delegate is always bound, this is not necessary.
///
/// \note If you are wondering where the code is, the delegate is implemented with macro and template magic in
/// Delegate_inl.h and DelegateHelper_inl.h.
template <typename T, plUInt32 DataSize = 16>
struct plDelegate : public plDelegateBase
{
};

template <typename T>
struct plMakeDelegateHelper;

/// \brief A helper function to create delegates from function pointers.
///
/// \code{.cpp}
///   void foo() { }
///   auto delegate = plMakeDelegate(&foo);
/// \endcode
template <typename Function>
plDelegate<Function> plMakeDelegate(Function* pFunction);

/// \brief A helper function to create delegates from methods.
///
/// \code{.cpp}
///   class Example
///   {
///   public:
///     void foo() {}
///   };
///   Example instance;
///   auto delegate = plMakeDelegate(&Example::foo, &instance);
/// \endcode
template <typename Method, typename Class>
typename plMakeDelegateHelper<Method>::DelegateType plMakeDelegate(Method method, Class* pClass);

#include <Foundation/Types/Implementation/Delegate_inl.h>
