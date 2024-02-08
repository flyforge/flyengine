#include <Core/CorePCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plSurfaceInteractionAlignment, 2)
  PL_ENUM_CONSTANTS(plSurfaceInteractionAlignment::SurfaceNormal, plSurfaceInteractionAlignment::IncidentDirection, plSurfaceInteractionAlignment::ReflectedDirection)
  PL_ENUM_CONSTANTS(plSurfaceInteractionAlignment::ReverseSurfaceNormal, plSurfaceInteractionAlignment::ReverseIncidentDirection, plSurfaceInteractionAlignment::ReverseReflectedDirection)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_TYPE(plSurfaceInteraction, plNoBase, 1, plRTTIDefaultAllocator<plSurfaceInteraction>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Type", m_sInteractionType)->AddAttributes(new plDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    PL_ACCESSOR_PROPERTY("Prefab", GetPrefab, SetPrefab)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab", plDependencyFlags::Package)),
    PL_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new plExposedParametersAttribute("Prefab")),
    PL_ENUM_MEMBER_PROPERTY("Alignment", plSurfaceInteractionAlignment, m_Alignment),
    PL_MEMBER_PROPERTY("Deviation", m_Deviation)->AddAttributes(new plClampValueAttribute(plVariant(plAngle::MakeFromDegree(0.0f)), plVariant(plAngle::MakeFromDegree(90.0f)))),
    PL_MEMBER_PROPERTY("ImpulseThreshold", m_fImpulseThreshold),
    PL_MEMBER_PROPERTY("ImpulseScale", m_fImpulseScale)->AddAttributes(new plDefaultValueAttribute(1.0f)),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSurfaceResourceDescriptor, 3, plRTTIDefaultAllocator<plSurfaceResourceDescriptor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("BaseSurface", GetBaseSurfaceFile, SetBaseSurfaceFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface")),// package+thumbnail so that it forbids circular dependencies
    PL_MEMBER_PROPERTY("Restitution", m_fPhysicsRestitution)->AddAttributes(new plDefaultValueAttribute(0.25f)),
    PL_MEMBER_PROPERTY("StaticFriction", m_fPhysicsFrictionStatic)->AddAttributes(new plDefaultValueAttribute(0.6f)),
    PL_MEMBER_PROPERTY("DynamicFriction", m_fPhysicsFrictionDynamic)->AddAttributes(new plDefaultValueAttribute(0.4f)),
    PL_MEMBER_PROPERTY("SoundObstruction", m_fSoundObstruction)->AddAttributes(new plDefaultValueAttribute(0.5f)),
    PL_MEMBER_PROPERTY("GroundType", m_iGroundType)->AddAttributes(new plDefaultValueAttribute(-1), new plDynamicEnumAttribute("AiGroundType")),
    PL_MEMBER_PROPERTY("SoundObstruction", m_fSoundObstruction)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 1.0f)),
    PL_ACCESSOR_PROPERTY("OnCollideInteraction", GetCollisionInteraction, SetCollisionInteraction)->AddAttributes(new plDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    PL_ACCESSOR_PROPERTY("SlideReaction", GetSlideReactionPrefabFile, SetSlideReactionPrefabFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab", plDependencyFlags::Package)),
    PL_ACCESSOR_PROPERTY("RollReaction", GetRollReactionPrefabFile, SetRollReactionPrefabFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab", plDependencyFlags::Package)),
    PL_ARRAY_MEMBER_PROPERTY("Interactions", m_Interactions),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plSurfaceInteraction::SetPrefab(const char* szPrefab)
{
  plPrefabResourceHandle hPrefab;

  if (!plStringUtils::IsNullOrEmpty(szPrefab))
  {
    hPrefab = plResourceManager::LoadResource<plPrefabResource>(szPrefab);
  }

  m_hPrefab = hPrefab;
}

const char* plSurfaceInteraction::GetPrefab() const
{
  if (!m_hPrefab.IsValid())
    return "";

  return m_hPrefab.GetResourceID();
}

const plRangeView<const char*, plUInt32> plSurfaceInteraction::GetParameters() const
{
  return plRangeView<const char*, plUInt32>([]() -> plUInt32 { return 0; },
    [this]() -> plUInt32 { return m_Parameters.GetCount(); },
    [](plUInt32& ref_uiIt) { ++ref_uiIt; },
    [this](const plUInt32& uiIt) -> const char* { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void plSurfaceInteraction::SetParameter(const char* szKey, const plVariant& value)
{
  plHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != plInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void plSurfaceInteraction::RemoveParameter(const char* szKey)
{
  m_Parameters.RemoveAndCopy(plTempHashedString(szKey));
}

bool plSurfaceInteraction::GetParameter(const char* szKey, plVariant& out_value) const
{
  plUInt32 it = m_Parameters.Find(szKey);

  if (it == plInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

void plSurfaceResourceDescriptor::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;

  inout_stream >> uiVersion;
  PL_ASSERT_DEV(uiVersion <= 9, "Invalid version {0} for surface resource", uiVersion);

  inout_stream >> m_fPhysicsRestitution;
  inout_stream >> m_fPhysicsFrictionStatic;
  inout_stream >> m_fPhysicsFrictionDynamic;
  inout_stream >> m_fSoundObstruction;
  inout_stream >> m_hBaseSurface;

  if (uiVersion >= 9)
  {
    inout_stream >> m_fSoundObstruction;
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_sOnCollideInteraction;
  }

  if (uiVersion >= 7)
  {
    inout_stream >> m_sSlideInteractionPrefab;
    inout_stream >> m_sRollInteractionPrefab;
  }

  if (uiVersion > 2)
  {
    plUInt32 count = 0;
    inout_stream >> count;
    m_Interactions.SetCount(count);

    plStringBuilder sTemp;
    for (plUInt32 i = 0; i < count; ++i)
    {
      auto& ia = m_Interactions[i];

      inout_stream >> sTemp;
      ia.m_sInteractionType = sTemp;

      inout_stream >> ia.m_hPrefab;
      inout_stream >> ia.m_Alignment;
      inout_stream >> ia.m_Deviation;

      if (uiVersion >= 4)
      {
        inout_stream >> ia.m_fImpulseThreshold;
      }

      if (uiVersion >= 5)
      {
        inout_stream >> ia.m_fImpulseScale;
      }

      if (uiVersion >= 6)
      {
        plUInt8 uiNumParams;
        inout_stream >> uiNumParams;

        ia.m_Parameters.Clear();
        ia.m_Parameters.Reserve(uiNumParams);

        plHashedString key;
        plVariant value;

        for (plUInt32 i2 = 0; i2 < uiNumParams; ++i2)
        {
          inout_stream >> key;
          inout_stream >> value;

          ia.m_Parameters.Insert(key, value);
        }
      }
    }
  }

  if (uiVersion >= 8)
  {
    inout_stream >> m_iGroundType;
  }
}

void plSurfaceResourceDescriptor::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = 9;

  inout_stream << uiVersion;
  inout_stream << m_fPhysicsRestitution;
  inout_stream << m_fPhysicsFrictionStatic;
  inout_stream << m_fPhysicsFrictionDynamic;
  inout_stream << m_fSoundObstruction;
  inout_stream << m_hBaseSurface;

  // version 9
  inout_stream << m_fSoundObstruction;

  // version 4
  inout_stream << m_sOnCollideInteraction;

  // version 7
  inout_stream << m_sSlideInteractionPrefab;
  inout_stream << m_sRollInteractionPrefab;

  inout_stream << m_Interactions.GetCount();
  for (const auto& ia : m_Interactions)
  {
    inout_stream << ia.m_sInteractionType;
    inout_stream << ia.m_hPrefab;
    inout_stream << ia.m_Alignment;
    inout_stream << ia.m_Deviation;

    // version 4
    inout_stream << ia.m_fImpulseThreshold;

    // version 5
    inout_stream << ia.m_fImpulseScale;

    // version 6
    const plUInt8 uiNumParams = static_cast<plUInt8>(ia.m_Parameters.GetCount());
    inout_stream << uiNumParams;
    for (plUInt32 i = 0; i < uiNumParams; ++i)
    {
      inout_stream << ia.m_Parameters.GetKey(i);
      inout_stream << ia.m_Parameters.GetValue(i);
    }
  }

  // version 8
  inout_stream << m_iGroundType;
}

void plSurfaceResourceDescriptor::SetBaseSurfaceFile(const char* szFile)
{
  plSurfaceResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plSurfaceResource>(szFile);
  }

  m_hBaseSurface = hResource;
}

const char* plSurfaceResourceDescriptor::GetBaseSurfaceFile() const
{
  if (!m_hBaseSurface.IsValid())
    return "";

  return m_hBaseSurface.GetResourceID();
}

void plSurfaceResourceDescriptor::SetCollisionInteraction(const char* szName)
{
  m_sOnCollideInteraction.Assign(szName);
}

const char* plSurfaceResourceDescriptor::GetCollisionInteraction() const
{
  return m_sOnCollideInteraction.GetData();
}

void plSurfaceResourceDescriptor::SetSlideReactionPrefabFile(const char* szFile)
{
  m_sSlideInteractionPrefab.Assign(szFile);
}

const char* plSurfaceResourceDescriptor::GetSlideReactionPrefabFile() const
{
  return m_sSlideInteractionPrefab.GetData();
}

void plSurfaceResourceDescriptor::SetRollReactionPrefabFile(const char* szFile)
{
  m_sRollInteractionPrefab.Assign(szFile);
}

const char* plSurfaceResourceDescriptor::GetRollReactionPrefabFile() const
{
  return m_sRollInteractionPrefab.GetData();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plSurfaceResourceDescriptorPatch_1_2 : public plGraphPatch
{
public:
  plSurfaceResourceDescriptorPatch_1_2()
    : plGraphPatch("plSurfaceResourceDescriptor", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Base Surface", "BaseSurface");
    pNode->RenameProperty("Static Friction", "StaticFriction");
    pNode->RenameProperty("Dynamic Friction", "DynamicFriction");
  }
};

plSurfaceResourceDescriptorPatch_1_2 g_plSurfaceResourceDescriptorPatch_1_2;


PL_STATICLINK_FILE(Core, Core_Physics_Implementation_SurfaceResourceDescriptor);
