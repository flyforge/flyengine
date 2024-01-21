#include <Core/CorePCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plSurfaceInteractionAlignment, 2)
  PLASMA_ENUM_CONSTANTS(plSurfaceInteractionAlignment::SurfaceNormal, plSurfaceInteractionAlignment::IncidentDirection, plSurfaceInteractionAlignment::ReflectedDirection)
  PLASMA_ENUM_CONSTANTS(plSurfaceInteractionAlignment::ReverseSurfaceNormal, plSurfaceInteractionAlignment::ReverseIncidentDirection, plSurfaceInteractionAlignment::ReverseReflectedDirection)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plSurfaceInteraction, plNoBase, 1, plRTTIDefaultAllocator<plSurfaceInteraction>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Type", m_sInteractionType)->AddAttributes(new plDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    PLASMA_ACCESSOR_PROPERTY("Prefab", GetPrefab, SetPrefab)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab")),
    PLASMA_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new plExposedParametersAttribute("Prefab")),
    PLASMA_ENUM_MEMBER_PROPERTY("Alignment", plSurfaceInteractionAlignment, m_Alignment),
    PLASMA_MEMBER_PROPERTY("Deviation", m_Deviation)->AddAttributes(new plClampValueAttribute(plVariant(plAngle::Degree(0.0f)), plVariant(plAngle::Degree(90.0f)))),
    PLASMA_MEMBER_PROPERTY("ImpulseThreshold", m_fImpulseThreshold),
    PLASMA_MEMBER_PROPERTY("ImpulseScale", m_fImpulseScale)->AddAttributes(new plDefaultValueAttribute(1.0f)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSurfaceResourceDescriptor, 3, plRTTIDefaultAllocator<plSurfaceResourceDescriptor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("BaseSurface", GetBaseSurfaceFile, SetBaseSurfaceFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface")),
    PLASMA_MEMBER_PROPERTY("Restitution", m_fPhysicsRestitution)->AddAttributes(new plDefaultValueAttribute(0.25f)),
    PLASMA_MEMBER_PROPERTY("StaticFriction", m_fPhysicsFrictionStatic)->AddAttributes(new plDefaultValueAttribute(0.6f)),
    PLASMA_MEMBER_PROPERTY("DynamicFriction", m_fPhysicsFrictionDynamic)->AddAttributes(new plDefaultValueAttribute(0.4f)),
    PLASMA_MEMBER_PROPERTY("GroundType", m_iGroundType)->AddAttributes(new plDefaultValueAttribute(-1), new plDynamicEnumAttribute("AiGroundType")),
    PLASMA_MEMBER_PROPERTY("SoundObstruction", m_fSoundObstruction)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_ACCESSOR_PROPERTY("OnCollideInteraction", GetCollisionInteraction, SetCollisionInteraction)->AddAttributes(new plDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    PLASMA_ACCESSOR_PROPERTY("SlideReaction", GetSlideReactionPrefabFile, SetSlideReactionPrefabFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab")),
    PLASMA_ACCESSOR_PROPERTY("RollReaction", GetRollReactionPrefabFile, SetRollReactionPrefabFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab")),
    PLASMA_ARRAY_MEMBER_PROPERTY("Interactions", m_Interactions),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
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
    [](plUInt32& it) { ++it; },
    [this](const plUInt32& it) -> const char* { return m_Parameters.GetKey(it).GetString().GetData(); });
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

void plSurfaceResourceDescriptor::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;

  stream >> uiVersion;
  PLASMA_ASSERT_DEV(uiVersion <= 9, "Invalid version {0} for surface resource", uiVersion);

  stream >> m_fPhysicsRestitution;
  stream >> m_fPhysicsFrictionStatic;
  stream >> m_fPhysicsFrictionDynamic;
  stream >> m_hBaseSurface;

  if (uiVersion >= 8)
  {
    stream >> m_fSoundObstruction;
  }

  if (uiVersion >= 4)
  {
    stream >> m_sOnCollideInteraction;
  }

  if (uiVersion >= 7)
  {
    stream >> m_sSlideInteractionPrefab;
    stream >> m_sRollInteractionPrefab;
  }

  if (uiVersion > 2)
  {
    plUInt32 count = 0;
    stream >> count;
    m_Interactions.SetCount(count);

    plStringBuilder sTemp;
    for (plUInt32 i = 0; i < count; ++i)
    {
      auto& ia = m_Interactions[i];

      stream >> sTemp;
      ia.m_sInteractionType = sTemp;

      stream >> ia.m_hPrefab;
      stream >> ia.m_Alignment;
      stream >> ia.m_Deviation;

      if (uiVersion >= 4)
      {
        stream >> ia.m_fImpulseThreshold;
      }

      if (uiVersion >= 5)
      {
        stream >> ia.m_fImpulseScale;
      }

      if (uiVersion >= 6)
      {
        plUInt8 uiNumParams;
        stream >> uiNumParams;

        ia.m_Parameters.Clear();
        ia.m_Parameters.Reserve(uiNumParams);

        plHashedString key;
        plVariant value;

        for (plUInt32 i2 = 0; i2 < uiNumParams; ++i2)
        {
          stream >> key;
          stream >> value;

          ia.m_Parameters.Insert(key, value);
        }
      }
    }
  }

  if (uiVersion >= 9)
  {
    stream >> m_iGroundType;
  }
}

void plSurfaceResourceDescriptor::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = 9;

  stream << uiVersion;
  stream << m_fPhysicsRestitution;
  stream << m_fPhysicsFrictionStatic;
  stream << m_fPhysicsFrictionDynamic;
  stream << m_hBaseSurface;

  // version 8
  stream << m_fSoundObstruction;

  // version 4
  stream << m_sOnCollideInteraction;

  // version 7
  stream << m_sSlideInteractionPrefab;
  stream << m_sRollInteractionPrefab;

  stream << m_Interactions.GetCount();
  for (const auto& ia : m_Interactions)
  {
    stream << ia.m_sInteractionType;
    stream << ia.m_hPrefab;
    stream << ia.m_Alignment;
    stream << ia.m_Deviation;

    // version 4
    stream << ia.m_fImpulseThreshold;

    // version 5
    stream << ia.m_fImpulseScale;

    // version 6
    const plUInt8 uiNumParams = static_cast<plUInt8>(ia.m_Parameters.GetCount());
    stream << uiNumParams;
    for (plUInt32 i = 0; i < uiNumParams; ++i)
    {
      stream << ia.m_Parameters.GetKey(i);
      stream << ia.m_Parameters.GetValue(i);
    }
  }

  // version 8
  stream << m_iGroundType;
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

void plSurfaceResourceDescriptor::SetCollisionInteraction(const char* name)
{
  m_sOnCollideInteraction.Assign(name);
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

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Base Surface", "BaseSurface");
    pNode->RenameProperty("Static Friction", "StaticFriction");
    pNode->RenameProperty("Dynamic Friction", "DynamicFriction");
  }
};

plSurfaceResourceDescriptorPatch_1_2 g_plSurfaceResourceDescriptorPatch_1_2;
