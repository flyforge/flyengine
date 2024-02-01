#include <Core/CorePCH.h>

#include <Core/Collection/CollectionComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plCollectionComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Collection", GetCollectionFile, SetCollectionFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_AssetCollection", plDependencyFlags::Package)),
    PL_MEMBER_PROPERTY("RegisterNames", m_bRegisterNames),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Utilities"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCollectionComponent::plCollectionComponent() = default;
plCollectionComponent::~plCollectionComponent() = default;

void plCollectionComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hCollection;
  s << m_bRegisterNames;
}

void plCollectionComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hCollection;

  if (uiVersion >= 2)
  {
    s >> m_bRegisterNames;
  }
}

void plCollectionComponent::SetCollectionFile(const char* szFile)
{
  plCollectionResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plCollectionResource>(szFile);
    plResourceManager::PreloadResource(hResource);
  }

  SetCollection(hResource);
}

const char* plCollectionComponent::GetCollectionFile() const
{
  if (!m_hCollection.IsValid())
    return {};

  return m_hCollection.GetResourceID();
}

void plCollectionComponent::SetCollection(const plCollectionResourceHandle& hCollection)
{
  m_hCollection = hCollection;

  if (IsActiveAndSimulating())
  {
    InitiatePreload();
  }
}

void plCollectionComponent::OnSimulationStarted()
{
  InitiatePreload();
}

void plCollectionComponent::InitiatePreload()
{
  if (m_hCollection.IsValid())
  {
    plResourceLock<plCollectionResource> pCollection(m_hCollection, plResourceAcquireMode::BlockTillLoaded_NeverFail);

    if (pCollection.GetAcquireResult() == plResourceAcquireResult::Final)
    {
      pCollection->PreloadResources();

      if (m_bRegisterNames)
      {
        pCollection->RegisterNames();
      }
    }
  }
}

PL_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionComponent);
