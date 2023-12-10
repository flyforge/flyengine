
#include <Foundation/Types/Implementation/DelegateHelper_inl.h>

template <typename Function>
PLASMA_ALWAYS_INLINE plDelegate<Function> plMakeDelegate(Function* pFunction)
{
  return plDelegate<Function>(pFunction);
}

template <typename Method, typename Class>
PLASMA_ALWAYS_INLINE typename plMakeDelegateHelper<Method>::DelegateType plMakeDelegate(Method method, Class* pClass)
{
  return typename plMakeDelegateHelper<Method>::DelegateType(method, pClass);
}
