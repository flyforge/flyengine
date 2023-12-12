#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/MessageHandler.h>

#include <Foundation/Communication/Message.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>

struct plTypeData
{
  plMutex m_Mutex;
  plHashTable<plUInt64, plRTTI*, plHashHelper<plUInt64>, plStaticAllocatorWrapper> m_TypeNameHashToType;
  plDynamicArray<plRTTI*> m_AllTypes;

  bool m_bIterating = false;
};

plTypeData* GetTypeData()
{
  // Prevent static initialization hazard between first plRTTI instance
  // and type data and also make sure it is sufficiently sized before first use.
  auto CreateData = []() -> plTypeData* {
    plTypeData* pData = new plTypeData();
    pData->m_TypeNameHashToType.Reserve(512);
    pData->m_AllTypes.Reserve(512);
    return pData;
  };
  static plTypeData* pData = CreateData();
  return pData;
}

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Reflection)

  //BEGIN_SUBSYSTEM_DEPENDENCIES
  //  "FileSystem"
  //END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plPlugin::Events().AddEventHandler(plRTTI::PluginEventHandler);
    plRTTI::AssignPlugin("Static");
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plPlugin::Events().RemoveEventHandler(plRTTI::PluginEventHandler);
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plRTTI::plRTTI(plStringView sName, const plRTTI* pParentType, plUInt32 uiTypeSize, plUInt32 uiTypeVersion, plUInt8 uiVariantType,
  plBitflags<plTypeFlags> flags, plRTTIAllocator* pAllocator, plArrayPtr<const plAbstractProperty*> properties, plArrayPtr<const plAbstractFunctionProperty*> functions,
  plArrayPtr<const plPropertyAttribute*> attributes, plArrayPtr<plAbstractMessageHandler*> messageHandlers, plArrayPtr<plMessageSenderInfo> messageSenders,
  const plRTTI* (*fnVerifyParent)())
  : m_sTypeName(sName)
  , m_Properties(properties)
  , m_Functions(functions)
  , m_Attributes(attributes)
  , m_pAllocator(pAllocator)
  , m_VerifyParent(fnVerifyParent)
  , m_MessageHandlers(messageHandlers)
  , m_MessageSenders(messageSenders)
{
  UpdateType(pParentType, uiTypeSize, uiTypeVersion, uiVariantType, flags);

  // This part is not guaranteed to always work here!
  // pParentType is (apparently) always the correct pointer to the base class BUT it is not guaranteed to have been constructed at this
  // point in time! Therefore the message handler hierarchy is initialized delayed in DispatchMessage
  //
  // However, I don't know where we could do these debug checks where they are guaranteed to be executed.
  // For now they are executed here and one might also do that in e.g. the game application
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
    VerifyCorrectness();
#endif
  }

  if (!m_sTypeName.IsEmpty())
  {
    RegisterType();
  }
}

plRTTI::~plRTTI()
{
  if (!m_sTypeName.IsEmpty())
  {
    UnregisterType();
  }
}

void plRTTI::GatherDynamicMessageHandlers()
{
  // This cannot be done in the constructor, because the parent types are not guaranteed to be initialized at that point

  if (m_uiMsgIdOffset != plSmallInvalidIndex)
    return;

  m_uiMsgIdOffset = 0;

  plUInt16 uiMinMsgId = plSmallInvalidIndex;
  plUInt16 uiMaxMsgId = 0;

  const plRTTI* pInstance = this;
  while (pInstance != nullptr)
  {
    for (plUInt32 i = 0; i < pInstance->m_MessageHandlers.GetCount(); ++i)
    {
      plUInt16 id = pInstance->m_MessageHandlers[i]->GetMessageId();
      uiMinMsgId = plMath::Min(uiMinMsgId, id);
      uiMaxMsgId = plMath::Max(uiMaxMsgId, id);
    }

    pInstance = pInstance->m_pParentType;
  }

  if (uiMinMsgId != plSmallInvalidIndex)
  {
    m_uiMsgIdOffset = uiMinMsgId;
    plUInt16 uiNeededCapacity = uiMaxMsgId - uiMinMsgId + 1;

    m_DynamicMessageHandlers.SetCount(uiNeededCapacity);

    pInstance = this;
    while (pInstance != nullptr)
    {
      for (plUInt32 i = 0; i < pInstance->m_MessageHandlers.GetCount(); ++i)
      {
        plAbstractMessageHandler* pHandler = pInstance->m_MessageHandlers[i];
        plUInt16 uiIndex = pHandler->GetMessageId() - m_uiMsgIdOffset;

        // this check ensures that handlers in base classes do not override the derived handlers
        if (m_DynamicMessageHandlers[uiIndex] == nullptr)
        {
          m_DynamicMessageHandlers[uiIndex] = pHandler;
        }
      }

      pInstance = pInstance->m_pParentType;
    }
  }
}

void plRTTI::SetupParentHierarchy()
{
  m_ParentHierarchy.Clear();

  for (const plRTTI* rtti = this; rtti != nullptr; rtti = rtti->m_pParentType)
  {
    m_ParentHierarchy.PushBack(rtti);
  }
}

void plRTTI::VerifyCorrectness() const
{
  if (m_VerifyParent != nullptr)
  {
    PLASMA_ASSERT_DEV(m_VerifyParent() == m_pParentType, "Type '{0}': The given parent type '{1}' does not match the actual parent type '{2}'",
      m_sTypeName, (m_pParentType != nullptr) ? m_pParentType->GetTypeName() : "null",
      (m_VerifyParent() != nullptr) ? m_VerifyParent()->GetTypeName() : "null");
  }

  {
    plSet<plStringView> Known;

    const plRTTI* pInstance = this;

    while (pInstance != nullptr)
    {
      for (plUInt32 i = 0; i < pInstance->m_Properties.GetCount(); ++i)
      {
        const bool bNewProperty = !Known.Find(pInstance->m_Properties[i]->GetPropertyName()).IsValid();
        Known.Insert(pInstance->m_Properties[i]->GetPropertyName());

        PLASMA_ASSERT_DEV(bNewProperty, "{0}: The property with name '{1}' is already defined in type '{2}'.", m_sTypeName,
          pInstance->m_Properties[i]->GetPropertyName(), pInstance->GetTypeName());
      }

      pInstance = pInstance->m_pParentType;
    }
  }

  {
    for (const plAbstractProperty* pFunc : m_Functions)
    {
      PLASMA_ASSERT_DEV(pFunc->GetCategory() == plPropertyCategory::Function, "Invalid function property '{}'", pFunc->GetPropertyName());
    }
  }
}

void plRTTI::VerifyCorrectnessForAllTypes()
{
  plRTTI::ForEachType([](const plRTTI* pRtti) { pRtti->VerifyCorrectness(); });
}


void plRTTI::UpdateType(const plRTTI* pParentType, plUInt32 uiTypeSize, plUInt32 uiTypeVersion, plUInt8 uiVariantType, plBitflags<plTypeFlags> flags)
{
  m_pParentType = pParentType;
  m_uiVariantType = uiVariantType;
  m_uiTypeSize = uiTypeSize;
  m_uiTypeVersion = uiTypeVersion;
  m_TypeFlags = flags;
  m_ParentHierarchy.Clear();
}

void plRTTI::RegisterType()
{
  m_uiTypeNameHash = plHashingUtils::StringHash(m_sTypeName);

  auto pData = GetTypeData();
  PLASMA_LOCK(pData->m_Mutex);
  pData->m_TypeNameHashToType.Insert(m_uiTypeNameHash, this);

  m_uiTypeIndex = pData->m_AllTypes.GetCount();
  pData->m_AllTypes.PushBack(this);
}

void plRTTI::UnregisterType()
{
  auto pData = GetTypeData();
  PLASMA_LOCK(pData->m_Mutex);
  pData->m_TypeNameHashToType.Remove(m_uiTypeNameHash);

  PLASMA_ASSERT_DEV(pData->m_bIterating == false, "Unregistering types while iterating over types might cause unexpected behavior");
  pData->m_AllTypes.RemoveAtAndSwap(m_uiTypeIndex);
  if (m_uiTypeIndex != pData->m_AllTypes.GetCount())
  {
    pData->m_AllTypes[m_uiTypeIndex]->m_uiTypeIndex = m_uiTypeIndex;
  }
}

void plRTTI::GetAllProperties(plDynamicArray<const plAbstractProperty*>& out_properties) const
{
  out_properties.Clear();

  if (m_pParentType)
    m_pParentType->GetAllProperties(out_properties);

  out_properties.PushBackRange(GetProperties());
}

const plRTTI* plRTTI::FindTypeByName(plStringView sName)
{
  plUInt64 uiNameHash = plHashingUtils::StringHash(sName);

  auto pData = GetTypeData();
  PLASMA_LOCK(pData->m_Mutex);

  plRTTI* pType = nullptr;
  pData->m_TypeNameHashToType.TryGetValue(uiNameHash, pType);
  return pType;
}

const plRTTI* plRTTI::FindTypeByNameHash(plUInt64 uiNameHash)
{
  auto pData = GetTypeData();
  PLASMA_LOCK(pData->m_Mutex);

  plRTTI* pType = nullptr;
  pData->m_TypeNameHashToType.TryGetValue(uiNameHash, pType);
  return pType;
}

const plRTTI* plRTTI::FindTypeByNameHash32(plUInt32 uiNameHash)
{
  return FindTypeIf([=](const plRTTI* pRtti) { return (plHashingUtils::StringHashTo32(pRtti->GetTypeNameHash()) == uiNameHash); });
}

const plRTTI* plRTTI::FindTypeIf(PredicateFunc func)
{
  auto pData = GetTypeData();
  PLASMA_LOCK(pData->m_Mutex);

  for (const plRTTI* pRtti : pData->m_AllTypes)
  {
    if (func(pRtti))
    {
      return pRtti;
    }
  }

  return nullptr;
}

const plAbstractProperty* plRTTI::FindPropertyByName(plStringView sName, bool bSearchBaseTypes /* = true */) const
{
  const plRTTI* pInstance = this;

  do
  {
    for (plUInt32 p = 0; p < pInstance->m_Properties.GetCount(); ++p)
    {
      if (pInstance->m_Properties[p]->GetPropertyName() == sName)
      {
        return pInstance->m_Properties[p];
      }
    }

    if (!bSearchBaseTypes)
      return nullptr;

    pInstance = pInstance->m_pParentType;
  } while (pInstance != nullptr);

  return nullptr;
}

bool plRTTI::DispatchMessage(void* pInstance, plMessage& ref_msg) const
{
  PLASMA_ASSERT_DEBUG(m_uiMsgIdOffset != plSmallInvalidIndex, "Message handler table should have been gathered at this point.\n"
                                                          "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                          "you may have forgotten to instantiate an plPlugin object inside your plugin DLL.");

  const plUInt32 uiIndex = ref_msg.GetId() - m_uiMsgIdOffset;

  // m_DynamicMessageHandlers contains all message handlers of this type and all base types
  if (uiIndex < m_DynamicMessageHandlers.GetCount())
  {
    plAbstractMessageHandler* pHandler = m_DynamicMessageHandlers.GetData()[uiIndex];
    if (pHandler != nullptr)
    {
      (*pHandler)(pInstance, ref_msg);
      return true;
    }
  }

  return false;
}

bool plRTTI::DispatchMessage(const void* pInstance, plMessage& ref_msg) const
{
  PLASMA_ASSERT_DEBUG(m_uiMsgIdOffset != plSmallInvalidIndex, "Message handler table should have been gathered at this point.\n"
                                                          "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                          "you may have forgotten to instantiate an plPlugin object inside your plugin DLL.");

  const plUInt32 uiIndex = ref_msg.GetId() - m_uiMsgIdOffset;

  // m_DynamicMessageHandlers contains all message handlers of this type and all base types
  if (uiIndex < m_DynamicMessageHandlers.GetCount())
  {
    plAbstractMessageHandler* pHandler = m_DynamicMessageHandlers.GetData()[uiIndex];
    if (pHandler != nullptr && pHandler->IsConst())
    {
      (*pHandler)(pInstance, ref_msg);
      return true;
    }
  }

  return false;
}

void plRTTI::ForEachType(VisitorFunc func, plBitflags<ForEachOptions> options /*= ForEachOptions::Default*/)
{
  auto pData = GetTypeData();
  PLASMA_LOCK(pData->m_Mutex);

  pData->m_bIterating = true;
  // Can't use ranged based for loop here since we might add new types while iterating and the m_AllTypes array might re-allocate.
  for (plUInt32 i = 0; i < pData->m_AllTypes.GetCount(); ++i) 
  {
    auto pRtti = pData->m_AllTypes.GetData()[i];
    if (options.IsSet(ForEachOptions::ExcludeNonAllocatable) && (pRtti->GetAllocator() == nullptr || pRtti->GetAllocator()->CanAllocate() == false))
      continue;

    if (options.IsSet(ForEachOptions::ExcludeAbstract) && pRtti->GetTypeFlags().IsSet(plTypeFlags::Abstract))
      continue;

    func(pRtti);
  }
  pData->m_bIterating = false;
}

void plRTTI::ForEachDerivedType(const plRTTI* pBaseType, VisitorFunc func, plBitflags<ForEachOptions> options /*= ForEachOptions::Default*/)
{
  auto pData = GetTypeData();
  PLASMA_LOCK(pData->m_Mutex);

  pData->m_bIterating = true;
  // Can't use ranged based for loop here since we might add new types while iterating and the m_AllTypes array might re-allocate.
  for (plUInt32 i = 0; i < pData->m_AllTypes.GetCount(); ++i)
  {
    auto pRtti = pData->m_AllTypes.GetData()[i];
    if (!pRtti->IsDerivedFrom(pBaseType))
      continue;

    if (options.IsSet(ForEachOptions::ExcludeNonAllocatable) && (pRtti->GetAllocator() == nullptr || pRtti->GetAllocator()->CanAllocate() == false))
      continue;

    if (options.IsSet(ForEachOptions::ExcludeAbstract) && pRtti->GetTypeFlags().IsSet(plTypeFlags::Abstract))
      continue;

    func(pRtti);
  }
  pData->m_bIterating = false;
}

void plRTTI::AssignPlugin(plStringView sPluginName)
{
  // assigns the given plugin name to every plRTTI instance that has no plugin assigned yet

  auto pData = GetTypeData();
  PLASMA_LOCK(pData->m_Mutex);

  for (plRTTI* pRtti : pData->m_AllTypes)
  {
    if (pRtti->m_sPluginName.IsEmpty())
    {
      pRtti->m_sPluginName = sPluginName;
      SanityCheckType(pRtti);

      pRtti->SetupParentHierarchy();
      pRtti->GatherDynamicMessageHandlers();
    }
  }
}

// warning C4505: 'IsValidIdentifierName': unreferenced function with internal linkage has been removed
// happens in Release builds, because the function is only used in a debug assert
#define PLASMA_MSVC_WARNING_NUMBER 4505
#include <Foundation/Basics/Compiler/MSVC/DisableWarning_MSVC.h>

static bool IsValidIdentifierName(plStringView sIdentifier)
{
  // empty strings are not valid
  if (sIdentifier.IsEmpty())
    return false;

  // digits are not allowed as the first character
  plUInt32 uiChar = sIdentifier.GetCharacter();
  if (uiChar >= '0' && uiChar <= '9')
    return false;

  for (auto it = sIdentifier.GetIteratorFront(); it.IsValid(); ++it)
  {
    const plUInt32 c = it.GetCharacter();

    if (c >= 'a' && c <= 'z')
      continue;
    if (c >= 'A' && c <= 'Z')
      continue;
    if (c >= '0' && c <= '9')
      continue;
    if (c >= '_')
      continue;
    if (c >= ':')
      continue;

    return false;
  }

  return true;
}

#include <Foundation/Basics/Compiler/MSVC/RestoreWarning_MSVC.h>

void plRTTI::SanityCheckType(plRTTI* pType)
{
  PLASMA_ASSERT_DEV(pType->GetTypeFlags().IsSet(plTypeFlags::StandardType) + pType->GetTypeFlags().IsSet(plTypeFlags::IsEnum) +
                    pType->GetTypeFlags().IsSet(plTypeFlags::Bitflags) + pType->GetTypeFlags().IsSet(plTypeFlags::Class) ==
                  1,
    "Types are mutually exclusive!");

  for (auto pProp : pType->m_Properties)
  {
    const plRTTI* pSpecificType = pProp->GetSpecificType();

    PLASMA_ASSERT_DEBUG(IsValidIdentifierName(pProp->GetPropertyName()), "Property name is invalid: '{0}'", pProp->GetPropertyName());

    if (pProp->GetCategory() != plPropertyCategory::Function)
    {
      PLASMA_ASSERT_DEV(pProp->GetFlags().IsSet(plPropertyFlags::StandardType) + pProp->GetFlags().IsSet(plPropertyFlags::IsEnum) +
                        pProp->GetFlags().IsSet(plPropertyFlags::Bitflags) + pProp->GetFlags().IsSet(plPropertyFlags::Class) <=
                      1,
        "Types are mutually exclusive!");
    }

    switch (pProp->GetCategory())
    {
      case plPropertyCategory::Constant:
      {
        PLASMA_ASSERT_DEV(pSpecificType->GetTypeFlags().IsSet(plTypeFlags::StandardType), "Only standard type constants are supported!");
      }
      break;
      case plPropertyCategory::Member:
      {
        PLASMA_ASSERT_DEV(pProp->GetFlags().IsSet(plPropertyFlags::StandardType) == pSpecificType->GetTypeFlags().IsSet(plTypeFlags::StandardType),
          "Property-Type missmatch!");
        PLASMA_ASSERT_DEV(pProp->GetFlags().IsSet(plPropertyFlags::IsEnum) == pSpecificType->GetTypeFlags().IsSet(plTypeFlags::IsEnum),
          "Property-Type missmatch! Use PLASMA_BEGIN_STATIC_REFLECTED_ENUM for type and PLASMA_ENUM_MEMBER_PROPERTY / "
          "PLASMA_ENUM_ACCESSOR_PROPERTY for property.");
        PLASMA_ASSERT_DEV(pProp->GetFlags().IsSet(plPropertyFlags::Bitflags) == pSpecificType->GetTypeFlags().IsSet(plTypeFlags::Bitflags),
          "Property-Type missmatch! Use PLASMA_BEGIN_STATIC_REFLECTED_ENUM for type and PLASMA_BITFLAGS_MEMBER_PROPERTY / "
          "PLASMA_BITFLAGS_ACCESSOR_PROPERTY for property.");
        PLASMA_ASSERT_DEV(pProp->GetFlags().IsSet(plPropertyFlags::Class) == pSpecificType->GetTypeFlags().IsSet(plTypeFlags::Class),
          "If plPropertyFlags::Class is set, the property type must be plTypeFlags::Class and vise versa.");
      }
      break;
      case plPropertyCategory::Array:
      case plPropertyCategory::Set:
      case plPropertyCategory::Map:
      {
        PLASMA_ASSERT_DEV(pProp->GetFlags().IsSet(plPropertyFlags::StandardType) == pSpecificType->GetTypeFlags().IsSet(plTypeFlags::StandardType),
          "Property-Type missmatch!");
        PLASMA_ASSERT_DEV(pProp->GetFlags().IsSet(plPropertyFlags::Class) == pSpecificType->GetTypeFlags().IsSet(plTypeFlags::Class),
          "If plPropertyFlags::Class is set, the property type must be plTypeFlags::Class and vise versa.");
      }
      break;
      case plPropertyCategory::Function:
        PLASMA_REPORT_FAILURE("Functions need to be put into the PLASMA_BEGIN_FUNCTIONS / PLASMA_END_FUNCTIONS; block.");
        break;
    }
  }
}

void plRTTI::PluginEventHandler(const plPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case plPluginEvent::BeforeLoading:
    {
      // before a new plugin is loaded, make sure all current plRTTI instances
      // are assigned to the proper plugin
      // all not-yet assigned rtti instances cannot be in any plugin, so assign them to the 'static' plugin
      AssignPlugin("Static");
    }
    break;

    case plPluginEvent::AfterLoadingBeforeInit:
    {
      // after we loaded a new plugin, but before it is initialized,
      // find all new rtti instances and assign them to that new plugin
      AssignPlugin(EventData.m_sPluginBinary);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
      plRTTI::VerifyCorrectnessForAllTypes();
#endif
    }
    break;

    default:
      break;
  }
}

plRTTIAllocator::~plRTTIAllocator() = default;


PLASMA_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_RTTI);
