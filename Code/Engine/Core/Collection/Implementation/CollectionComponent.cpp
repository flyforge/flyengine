#include <Core/CorePCH.h>

#include <Core/Collection/CollectionComponent.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plCollectionComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Collection", GetCollectionFile, SetCollectionFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_AssetCollection")),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("General"),
    new plColorAttribute(plColorScheme::Utilities),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCollectionComponent::plCollectionComponent() = default;
plCollectionComponent::~plCollectionComponent() = default;

void plCollectionComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hCollection;
}

void plCollectionComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hCollection;
}

void plCollectionComponent::SetCollectionFile(plStringView sFile)
{
  plCollectionResourceHandle hResource;

  if (!sFile.IsEmpty())
  {
    hResource = plResourceManager::LoadResource<plCollectionResource>(sFile);
    plResourceManager::PreloadResource(hResource);
  }

  SetCollection(hResource);
}

plStringView plCollectionComponent::GetCollectionFile() const
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
    }
  }
}

PLASMA_STATICLINK_FILE(Core, Core_Collection_Implementation_CollectionComponent);
