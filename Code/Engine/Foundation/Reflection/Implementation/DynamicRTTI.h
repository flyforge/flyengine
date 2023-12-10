#pragma once

/// \file

#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// \brief This needs to be put into the class declaration of EVERY dynamically reflectable class.
///
/// This macro extends a class, such that it is now able to return its own type information via GetDynamicRTTI(),
/// which is a virtual function, that is reimplemented on each type. A class needs to be derived from plReflectedClass
/// (at least indirectly) for this.
#define PLASMA_ADD_DYNAMIC_REFLECTION_NO_GETTER(SELF, BASE_TYPE) \
  PLASMA_ALLOW_PRIVATE_PROPERTIES(SELF);                         \
                                                             \
public:                                                      \
  using OWNTYPE = SELF;                                      \
  using SUPER = BASE_TYPE;                                   \
  PLASMA_ALWAYS_INLINE static const plRTTI* GetStaticRTTI()      \
  {                                                          \
    return &SELF::s_RTTI;                                    \
  }                                                          \
                                                             \
private:                                                     \
  static plRTTI s_RTTI;                                      \
  PLASMA_REFLECTION_DEBUG_CODE


#define PLASMA_ADD_DYNAMIC_REFLECTION(SELF, BASE_TYPE)      \
  PLASMA_ADD_DYNAMIC_REFLECTION_NO_GETTER(SELF, BASE_TYPE)  \
public:                                                 \
  virtual const plRTTI* GetDynamicRTTI() const override \
  {                                                     \
    return &SELF::s_RTTI;                               \
  }


#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT) && PLASMA_ENABLED(PLASMA_COMPILER_MSVC)

#  define PLASMA_REFLECTION_DEBUG_CODE                       \
    static const plRTTI* ReflectionDebug_GetParentType() \
    {                                                    \
      return __super::GetStaticRTTI();                   \
    }

#  define PLASMA_REFLECTION_DEBUG_GETPARENTFUNC &OwnType::ReflectionDebug_GetParentType

#else
#  define PLASMA_REFLECTION_DEBUG_CODE /*empty*/
#  define PLASMA_REFLECTION_DEBUG_GETPARENTFUNC nullptr
#endif


/// \brief Implements the necessary functionality for a type to be dynamically reflectable.
///
/// \param Type
///   The type for which the reflection functionality should be implemented.
/// \param BaseType
///   The base class type of \a Type. If it has no base class, pass plNoBase
/// \param AllocatorType
///   The type of an plRTTIAllocator that can be used to create and destroy instances
///   of \a Type. Pass plRTTINoAllocator for types that should not be created dynamically.
///   Pass plRTTIDefaultAllocator<Type> for types that should be created on the default heap.
///   Pass a custom plRTTIAllocator type to handle allocation differently.
#define PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(Type, Version, AllocatorType) \
  PLASMA_RTTIINFO_DECL(Type, Type::SUPER, Version)                        \
  plRTTI Type::s_RTTI = GetRTTI((Type*)0);                            \
  PLASMA_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, Type::SUPER, AllocatorType)

/// \brief Ends the reflection code block that was opened with PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE.
#define PLASMA_END_DYNAMIC_REFLECTED_TYPE                                                                                                \
  return plRTTI(GetTypeName((OwnType*)0), plGetStaticRTTI<OwnBaseType>(), sizeof(OwnType), GetTypeVersion((OwnType*)0),              \
    plVariant::TypeDeduction<OwnType>::value, flags, &Allocator, Properties, Functions, Attributes, MessageHandlers, MessageSenders, \
    PLASMA_REFLECTION_DEBUG_GETPARENTFUNC);                                                                                              \
  }

/// \brief All classes that should be dynamically reflectable, need to be derived from this base class.
class PLASMA_FOUNDATION_DLL plReflectedClass : public plNoBase
{
  PLASMA_ADD_DYNAMIC_REFLECTION_NO_GETTER(plReflectedClass, plNoBase);

public:
  virtual const plRTTI* GetDynamicRTTI() const { return &plReflectedClass::s_RTTI; }

public:
  PLASMA_ALWAYS_INLINE plReflectedClass() = default;
  PLASMA_ALWAYS_INLINE virtual ~plReflectedClass() = default;

  /// \brief Returns whether the type of this instance is of the given type or derived from it.
  bool IsInstanceOf(const plRTTI* pType) const;

  /// \brief Returns whether the type of this instance is of the given type or derived from it.
  template <typename T>
  PLASMA_ALWAYS_INLINE bool IsInstanceOf() const
  {
    const plRTTI* pType = plGetStaticRTTI<T>();
    return IsInstanceOf(pType);
  }
};
