#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

class plSurfaceResource;
class plComponent;
class plJoltDynamicActorComponent;
class plJoltStaticActorComponent;
class plJoltTriggerComponent;
class plJoltCharacterControllerComponent;
class plJoltShapeComponent;
class plJoltQueryShapeActorComponent;
class plJoltRagdollComponent;
class plJoltRopeComponent;
class plJoltActorComponent;

class plJoltUserData
{
public:
  PLASMA_DECLARE_POD_TYPE();

  plJoltUserData() = default;
  ~plJoltUserData() = default;

  PLASMA_ALWAYS_INLINE void Init(plJoltDynamicActorComponent* pObject)
  {
    m_Type = Type::DynamicActorComponent;
    m_pObject = pObject;
  }

  PLASMA_ALWAYS_INLINE void Init(plJoltStaticActorComponent* pObject)
  {
    m_Type = Type::StaticActorComponent;
    m_pObject = pObject;
  }

  PLASMA_ALWAYS_INLINE void Init(plJoltTriggerComponent* pObject)
  {
    m_Type = Type::TriggerComponent;
    m_pObject = pObject;
  }

  PLASMA_ALWAYS_INLINE void Init(plJoltCharacterControllerComponent* pObject)
  {
    m_Type = Type::CharacterComponent;
    m_pObject = pObject;
  }

  PLASMA_ALWAYS_INLINE void Init(plJoltShapeComponent* pObject)
  {
    m_Type = Type::ShapeComponent;
    m_pObject = pObject;
  }

  PLASMA_ALWAYS_INLINE void Init(plJoltQueryShapeActorComponent* pObject)
  {
    m_Type = Type::QueryShapeActorComponent;
    m_pObject = pObject;
  }

  PLASMA_ALWAYS_INLINE void Init(plSurfaceResource* pObject)
  {
    m_Type = Type::SurfaceResource;
    m_pObject = pObject;
  }

  PLASMA_ALWAYS_INLINE void Init(plJoltRagdollComponent* pObject)
  {
    m_Type = Type::RagdollComponent;
    m_pObject = pObject;
  }

  PLASMA_ALWAYS_INLINE void Init(plJoltRopeComponent* pObject)
  {
    m_Type = Type::RopeComponent;
    m_pObject = pObject;
  }

  PLASMA_FORCE_INLINE void Invalidate()
  {
    m_Type = Type::Invalid;
    m_pObject = nullptr;
  }

  PLASMA_FORCE_INLINE static plComponent* GetComponent(const void* pUserData)
  {
    const plJoltUserData* pJoltUserData = static_cast<const plJoltUserData*>(pUserData);
    if (pJoltUserData == nullptr ||
        pJoltUserData->m_Type == Type::Invalid ||
        pJoltUserData->m_Type == Type::SurfaceResource)
    {
      return nullptr;
    }

    return static_cast<plComponent*>(pJoltUserData->m_pObject);
  }

  PLASMA_FORCE_INLINE static plJoltActorComponent* GetActorComponent(const void* pUserData)
  {
    const plJoltUserData* pJoltUserData = static_cast<const plJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr &&
        (pJoltUserData->m_Type == Type::DynamicActorComponent ||
          pJoltUserData->m_Type == Type::StaticActorComponent ||
          pJoltUserData->m_Type == Type::QueryShapeActorComponent))
    {
      return static_cast<plJoltActorComponent*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

  PLASMA_FORCE_INLINE static plJoltDynamicActorComponent* GetDynamicActorComponent(const void* pUserData)
  {
    const plJoltUserData* pJoltUserData = static_cast<const plJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::DynamicActorComponent)
    {
      return static_cast<plJoltDynamicActorComponent*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

  PLASMA_FORCE_INLINE static plJoltRagdollComponent* GetRagdollComponent(const void* pUserData)
  {
    const plJoltUserData* pJoltUserData = static_cast<const plJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::RagdollComponent)
    {
      return static_cast<plJoltRagdollComponent*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

  PLASMA_FORCE_INLINE static plJoltRopeComponent* GetRopeComponent(const void* pUserData)
  {
    const plJoltUserData* pJoltUserData = static_cast<const plJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::RopeComponent)
    {
      return static_cast<plJoltRopeComponent*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

  // PLASMA_FORCE_INLINE static plJoltShapeComponent* GetShapeComponent(const void* pUserData)
  //{
  //   const plJoltUserData* pJoltUserData = static_cast<const plJoltUserData*>(pUserData);
  //   if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::ShapeComponent)
  //   {
  //     return static_cast<plJoltShapeComponent*>(pJoltUserData->m_pObject);
  //   }

  //  return nullptr;
  //}

  PLASMA_FORCE_INLINE static plJoltTriggerComponent* GetTriggerComponent(const void* pUserData)
  {
    const plJoltUserData* pJoltUserData = static_cast<const plJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::TriggerComponent)
    {
      return static_cast<plJoltTriggerComponent*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

  PLASMA_FORCE_INLINE static const plSurfaceResource* GetSurfaceResource(const void* pUserData)
  {
    const plJoltUserData* pJoltUserData = static_cast<const plJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::SurfaceResource)
    {
      return static_cast<const plSurfaceResource*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

private:
  enum class Type
  {
    Invalid,
    DynamicActorComponent,
    StaticActorComponent,
    TriggerComponent,
    CharacterComponent,
    ShapeComponent,
    BreakableSheetComponent,
    SurfaceResource,
    QueryShapeActorComponent,
    RagdollComponent,
    RopeComponent,
  };

  Type m_Type = Type::Invalid;
  void* m_pObject = nullptr;
};
