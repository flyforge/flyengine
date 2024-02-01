#include <Core/CorePCH.h>

#include <Core/Configuration/PlatformProfile.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>

#include <Core/ResourceManager/ResourceManager.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plProfileTargetPlatform, 1)
  PL_ENUM_CONSTANTS(plProfileTargetPlatform::PC, plProfileTargetPlatform::UWP, plProfileTargetPlatform::Android)
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProfileConfigData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

plProfileConfigData::plProfileConfigData() = default;
plProfileConfigData::~plProfileConfigData() = default;

void plProfileConfigData::SaveRuntimeData(plChunkStreamWriter& inout_stream) const {}
void plProfileConfigData::LoadRuntimeData(plChunkStreamReader& inout_stream) {}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plPlatformProfile, 1, plRTTIDefaultAllocator<plPlatformProfile>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new plHiddenAttribute()),
    PL_ENUM_MEMBER_PROPERTY("Platform", plProfileTargetPlatform, m_TargetPlatform),
    PL_ARRAY_MEMBER_PROPERTY("Configs", m_Configs)->AddFlags(plPropertyFlags::PointerOwner)->AddAttributes(new plContainerAttribute(false, false, false)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plPlatformProfile::plPlatformProfile() = default;

plPlatformProfile::~plPlatformProfile()
{
  Clear();
}

void plPlatformProfile::Clear()
{
  for (auto pType : m_Configs)
  {
    pType->GetDynamicRTTI()->GetAllocator()->Deallocate(pType);
  }

  m_Configs.Clear();
}

void plPlatformProfile::AddMissingConfigs()
{
  plRTTI::ForEachDerivedType<plProfileConfigData>(
    [this](const plRTTI* pRtti)
    {
      // find all types derived from plProfileConfigData
      bool bHasTypeAlready = false;

      // check whether we already have an instance of this type
      for (auto pType : m_Configs)
      {
        if (pType && pType->GetDynamicRTTI() == pRtti)
        {
          bHasTypeAlready = true;
          break;
        }
      }

      if (!bHasTypeAlready)
      {
        // if not, allocate one
        plProfileConfigData* pObject = pRtti->GetAllocator()->Allocate<plProfileConfigData>();
        PL_ASSERT_DEV(pObject != nullptr, "Invalid profile config");
        plReflectionUtils::SetAllMemberPropertiesToDefault(pRtti, pObject);

        m_Configs.PushBack(pObject);
      }
    },
    plRTTI::ForEachOptions::ExcludeNonAllocatable);

  // in case unknown configs were loaded from disk, remove them
  m_Configs.RemoveAndSwap(nullptr);

  // sort all configs alphabetically
  m_Configs.Sort([](const plProfileConfigData* lhs, const plProfileConfigData* rhs) -> bool
    { return lhs->GetDynamicRTTI()->GetTypeName().Compare(rhs->GetDynamicRTTI()->GetTypeName()) < 0; });
}

const plProfileConfigData* plPlatformProfile::GetTypeConfig(const plRTTI* pRtti) const
{
  for (const auto* pConfig : m_Configs)
  {
    if (pConfig->GetDynamicRTTI() == pRtti)
      return pConfig;
  }

  return nullptr;
}

plProfileConfigData* plPlatformProfile::GetTypeConfig(const plRTTI* pRtti)
{
  // reuse the const-version
  return const_cast<plProfileConfigData*>(((const plPlatformProfile*)this)->GetTypeConfig(pRtti));
}

plResult plPlatformProfile::SaveForRuntime(plStringView sFile) const
{
  plFileWriter file;
  PL_SUCCEED_OR_RETURN(file.Open(sFile));

  plChunkStreamWriter chunk(file);

  chunk.BeginStream(1);

  for (auto* pConfig : m_Configs)
  {
    pConfig->SaveRuntimeData(chunk);
  }

  chunk.EndStream();

  return PL_SUCCESS;
}

plResult plPlatformProfile::LoadForRuntime(plStringView sFile)
{
  plFileReader file;
  PL_SUCCEED_OR_RETURN(file.Open(sFile));

  plChunkStreamReader chunk(file);

  chunk.BeginStream();

  while (chunk.GetCurrentChunk().m_bValid)
  {
    for (auto* pConfig : m_Configs)
    {
      pConfig->LoadRuntimeData(chunk);
    }

    chunk.NextChunk();
  }

  chunk.EndStream();

  ++m_uiLastModificationCounter;
  return PL_SUCCESS;
}



PL_STATICLINK_FILE(Core, Core_Configuration_Implementation_PlatformProfile);
