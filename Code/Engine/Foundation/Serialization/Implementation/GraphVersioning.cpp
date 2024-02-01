#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/RttiConverter.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plTypeVersionInfo, plNoBase, 1, plRTTIDefaultAllocator<plTypeVersionInfo>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("TypeName", GetTypeName, SetTypeName),
    PL_ACCESSOR_PROPERTY("ParentTypeName", GetParentTypeName, SetParentTypeName),
    PL_MEMBER_PROPERTY("TypeVersion", m_uiTypeVersion),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

const char* plTypeVersionInfo::GetTypeName() const
{
  return m_sTypeName.GetData();
}

void plTypeVersionInfo::SetTypeName(const char* szName)
{
  m_sTypeName.Assign(szName);
}

const char* plTypeVersionInfo::GetParentTypeName() const
{
  return m_sParentTypeName.GetData();
}

void plTypeVersionInfo::SetParentTypeName(const char* szName)
{
  m_sParentTypeName.Assign(szName);
}

void plGraphPatchContext::PatchBaseClass(const char* szType, plUInt32 uiTypeVersion, bool bForcePatch)
{
  plHashedString sType;
  sType.Assign(szType);
  for (plUInt32 uiBaseClassIndex = m_uiBaseClassIndex; uiBaseClassIndex < m_BaseClasses.GetCount(); ++uiBaseClassIndex)
  {
    if (m_BaseClasses[uiBaseClassIndex].m_sType == sType)
    {
      Patch(uiBaseClassIndex, uiTypeVersion, bForcePatch);
      return;
    }
  }
  PL_REPORT_FAILURE("Base class of name '{0}' not found in parent types of '{1}'", sType.GetData(), m_pNode->GetType());
}

void plGraphPatchContext::RenameClass(const char* szTypeName)
{
  m_pNode->SetType(m_pGraph->RegisterString(szTypeName));
  m_BaseClasses[m_uiBaseClassIndex].m_sType.Assign(szTypeName);
}


void plGraphPatchContext::RenameClass(const char* szTypeName, plUInt32 uiVersion)
{
  m_pNode->SetType(m_pGraph->RegisterString(szTypeName));
  m_BaseClasses[m_uiBaseClassIndex].m_sType.Assign(szTypeName);
  // After a Patch is applied, the version is always increased. So if we want to change the version we need to reduce it by one so that in the next patch loop the requested version is not skipped.
  PL_ASSERT_DEV(uiVersion > 0, "Cannot change the version of a class to 0, target version must be at least 1.");
  m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion = uiVersion - 1;
}

void plGraphPatchContext::ChangeBaseClass(plArrayPtr<plVersionKey> baseClasses)
{
  m_BaseClasses.SetCount(m_uiBaseClassIndex + 1 + baseClasses.GetCount());
  for (plUInt32 i = 0; i < baseClasses.GetCount(); i++)
  {
    m_BaseClasses[m_uiBaseClassIndex + 1 + i] = baseClasses[i];
  }
}

//////////////////////////////////////////////////////////////////////////

plGraphPatchContext::plGraphPatchContext(plGraphVersioning* pParent, plAbstractObjectGraph* pGraph, plAbstractObjectGraph* pTypesGraph)
{
  PL_PROFILE_SCOPE("plGraphPatchContext");
  m_pParent = pParent;
  m_pGraph = pGraph;
  if (pTypesGraph)
  {
    plRttiConverterContext context;
    plRttiConverterReader rttiConverter(pTypesGraph, &context);
    plString sDescTypeName = "plReflectedTypeDescriptor";
    auto& nodes = pTypesGraph->GetAllNodes();
    m_TypeToInfo.Reserve(nodes.GetCount());
    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value()->GetType() == sDescTypeName)
      {
        plTypeVersionInfo info;
        rttiConverter.ApplyPropertiesToObject(it.Value(), plGetStaticRTTI<plTypeVersionInfo>(), &info);
        m_TypeToInfo.Insert(info.m_sTypeName, info);
      }
    }
  }
}

void plGraphPatchContext::Patch(plAbstractObjectNode* pNode)
{
  m_pNode = pNode;
  // Build version hierarchy.
  m_BaseClasses.Clear();
  plVersionKey key;
  key.m_sType.Assign(m_pNode->GetType());
  key.m_uiTypeVersion = m_pNode->GetTypeVersion();

  m_BaseClasses.PushBack(key);
  UpdateBaseClasses();

  // Patch
  for (m_uiBaseClassIndex = 0; m_uiBaseClassIndex < m_BaseClasses.GetCount(); ++m_uiBaseClassIndex)
  {
    const plUInt32 uiMaxVersion = m_pParent->GetMaxPatchVersion(m_BaseClasses[m_uiBaseClassIndex].m_sType);
    Patch(m_uiBaseClassIndex, uiMaxVersion, false);
  }
  m_pNode->SetTypeVersion(m_BaseClasses[0].m_uiTypeVersion);
}


void plGraphPatchContext::Patch(plUInt32 uiBaseClassIndex, plUInt32 uiTypeVersion, bool bForcePatch)
{
  if (bForcePatch)
  {
    m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion = plMath::Min(m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion, uiTypeVersion - 1);
  }
  while (m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion < uiTypeVersion)
  {
    // Don't move this out of the loop, needed to support renaming a class which will change the key.
    plVersionKey key = m_BaseClasses[uiBaseClassIndex];
    key.m_uiTypeVersion += 1;
    const plGraphPatch* pPatch = nullptr;
    if (m_pParent->m_NodePatches.TryGetValue(key, pPatch))
    {
      pPatch->Patch(*this, m_pGraph, m_pNode);
      uiTypeVersion = m_pParent->GetMaxPatchVersion(m_BaseClasses[m_uiBaseClassIndex].m_sType);
    }
    // Don't use a ref to the key as the array might get resized during patching.
    // Patch function can change the type and version so we need to read m_uiTypeVersion again instead of just writing key.m_uiTypeVersion;
    m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion++;
  }
}

void plGraphPatchContext::UpdateBaseClasses()
{
  for (;;)
  {
    plHashedString sParentType;
    if (plTypeVersionInfo* pInfo = m_TypeToInfo.GetValue(m_BaseClasses.PeekBack().m_sType))
    {
      m_BaseClasses.PeekBack().m_uiTypeVersion = pInfo->m_uiTypeVersion;
      sParentType = pInfo->m_sParentTypeName;
    }
    else if (const plRTTI* pType = plRTTI::FindTypeByName(m_BaseClasses.PeekBack().m_sType.GetData()))
    {
      m_BaseClasses.PeekBack().m_uiTypeVersion = pType->GetTypeVersion();
      if (pType->GetParentType())
      {
        sParentType.Assign(pType->GetParentType()->GetTypeName());
      }
      else
        sParentType = plHashedString();
    }
    else
    {
      plLog::Error("Can't patch base class, parent type of '{0}' unknown.", m_BaseClasses.PeekBack().m_sType.GetData());
      break;
    }

    if (sParentType.IsEmpty())
      break;

    plVersionKey key;
    key.m_sType = std::move(sParentType);
    key.m_uiTypeVersion = 0;
    m_BaseClasses.PushBack(key);
  }
}

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_SINGLETON(plGraphVersioning);

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(Foundation, GraphVersioning)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Reflection"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    PL_DEFAULT_NEW(plGraphVersioning);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plGraphVersioning* pDummy = plGraphVersioning::GetSingleton();
    PL_DEFAULT_DELETE(pDummy);
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

plGraphVersioning::plGraphVersioning()
  : m_SingletonRegistrar(this)
{
  plPlugin::Events().AddEventHandler(plMakeDelegate(&plGraphVersioning::PluginEventHandler, this));

  UpdatePatches();
}

plGraphVersioning::~plGraphVersioning()
{
  plPlugin::Events().RemoveEventHandler(plMakeDelegate(&plGraphVersioning::PluginEventHandler, this));
}

void plGraphVersioning::PatchGraph(plAbstractObjectGraph* pGraph, plAbstractObjectGraph* pTypesGraph)
{
  PL_PROFILE_SCOPE("PatchGraph");

  plGraphPatchContext context(this, pGraph, pTypesGraph);
  for (const plGraphPatch* pPatch : m_GraphPatches)
  {
    pPatch->Patch(context, pGraph, nullptr);
  }

  auto& nodes = pGraph->GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    plAbstractObjectNode* pNode = it.Value();
    context.Patch(pNode);
  }
}

void plGraphVersioning::PluginEventHandler(const plPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case plPluginEvent::AfterLoadingBeforeInit:
    case plPluginEvent::AfterUnloading:
      UpdatePatches();
      break;
    default:
      break;
  }
}

void plGraphVersioning::UpdatePatches()
{
  m_GraphPatches.Clear();
  m_NodePatches.Clear();
  m_MaxPatchVersion.Clear();

  plVersionKey key;
  plGraphPatch* pInstance = plGraphPatch::GetFirstInstance();

  while (pInstance)
  {
    switch (pInstance->GetPatchType())
    {
      case plGraphPatch::PatchType::NodePatch:
      {
        key.m_sType.Assign(pInstance->GetType());
        key.m_uiTypeVersion = pInstance->GetTypeVersion();
        m_NodePatches.Insert(key, pInstance);

        if (plUInt32* pMax = m_MaxPatchVersion.GetValue(key.m_sType))
        {
          *pMax = plMath::Max(*pMax, key.m_uiTypeVersion);
        }
        else
        {
          m_MaxPatchVersion[key.m_sType] = key.m_uiTypeVersion;
        }
      }
      break;
      case plGraphPatch::PatchType::GraphPatch:
      {
        m_GraphPatches.PushBack(pInstance);
      }
      break;
    }
    pInstance = pInstance->GetNextInstance();
  }

  m_GraphPatches.Sort([](const plGraphPatch* a, const plGraphPatch* b) -> bool { return a->GetTypeVersion() < b->GetTypeVersion(); });
}

plUInt32 plGraphVersioning::GetMaxPatchVersion(const plHashedString& sType) const
{
  if (const plUInt32* pMax = m_MaxPatchVersion.GetValue(sType))
  {
    return *pMax;
  }
  return 0;
}

PL_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_GraphVersioning);
