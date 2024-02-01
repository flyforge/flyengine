#include <Core/CorePCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plPrefabReferenceComponent, 4, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Prefab", GetPrefabFile, SetPrefabFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab")),
    PL_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new plExposedParametersAttribute("Prefab")),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Prefabs"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

enum PrefabComponentFlags
{
  SelfDeletion = 1, ///< the prefab component is currently deleting itself but does not want to remove the instantiated objects
};

plPrefabReferenceComponent::plPrefabReferenceComponent() = default;
plPrefabReferenceComponent::~plPrefabReferenceComponent() = default;

void plPrefabReferenceComponent::SerializePrefabParameters(const plWorld& world, plWorldWriter& inout_stream, plArrayMap<plHashedString, plVariant> parameters)
{
  // we need a copy of the parameters here, therefore we don't take it by reference

  auto& s = inout_stream.GetStream();
  const plUInt32 numParams = parameters.GetCount();

  plHybridArray<plGameObjectHandle, 8> GoReferences;

  // Version 4
  {
    // to support game object references as exposed parameters (which are currently exposed as strings)
    // we need to remap the string from an 'editor uuid' to something that can be interpreted as a proper plGameObjectHandle at runtime

    // so first we get the resolver and try to map any string parameter to a valid plGameObjectHandle
    auto resolver = world.GetGameObjectReferenceResolver();

    if (resolver.IsValid())
    {
      plStringBuilder tmp;

      for (plUInt32 i = 0; i < numParams; ++i)
      {
        // if this is a string parameter
        plVariant& var = parameters.GetValue(i);
        if (var.IsA<plString>())
        {
          // and the resolver CAN map this string to a game object handle
          plGameObjectHandle hObject = resolver(var.Get<plString>().GetData(), plComponentHandle(), nullptr);
          if (!hObject.IsInvalidated())
          {
            // write the handle properly to file (this enables correct remapping during deserialization)
            // and discard the string's value, and instead write a string that specifies the index of the serialized handle to use

            // local game object reference - index into GoReferences
            tmp.SetFormat("#!LGOR-{}", GoReferences.GetCount());
            var = tmp.GetData();

            GoReferences.PushBack(hObject);
          }
        }
      }
    }

    // now write all the plGameObjectHandle's such that during deserialization the plWorldReader will remap it as needed
    const plUInt8 numRefs = static_cast<plUInt8>(GoReferences.GetCount());
    s << numRefs;

    for (plUInt8 i = 0; i < numRefs; ++i)
    {
      inout_stream.WriteGameObjectHandle(GoReferences[i]);
    }
  }

  // Version 2
  s << numParams;
  for (plUInt32 i = 0; i < numParams; ++i)
  {
    s << parameters.GetKey(i);
    s << parameters.GetValue(i); // this may contain modified strings now, to map the game object handle references
  }
}

void plPrefabReferenceComponent::DeserializePrefabParameters(plArrayMap<plHashedString, plVariant>& out_parameters, plWorldReader& inout_stream)
{
  out_parameters.Clear();

  // versioning of this stuff is tied to the version number of plPrefabReferenceComponent
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(plGetStaticRTTI<plPrefabReferenceComponent>());
  auto& s = inout_stream.GetStream();

  // temp array to hold (and remap) the serialized game object handles
  plHybridArray<plGameObjectHandle, 8> GoReferences;

  if (uiVersion >= 4)
  {
    plUInt8 numRefs = 0;
    s >> numRefs;
    GoReferences.SetCountUninitialized(numRefs);

    // just read them all, this will remap as necessary to the plWorldReader
    for (plUInt8 i = 0; i < numRefs; ++i)
    {
      GoReferences[i] = inout_stream.ReadGameObjectHandle();
    }
  }

  if (uiVersion >= 2)
  {
    plUInt32 numParams = 0;
    s >> numParams;

    out_parameters.Reserve(numParams);

    plHashedString key;
    plVariant value;
    plStringBuilder tmp;

    for (plUInt32 i = 0; i < numParams; ++i)
    {
      s >> key;
      s >> value;

      if (value.IsA<plString>())
      {
        // if we find a string parameter, check if it is a 'local game object reference'
        const plString& str = value.Get<plString>();
        if (str.StartsWith("#!LGOR-"))
        {
          // if so, extract the index into the GoReferences array
          plInt32 idx;
          if (plConversionUtils::StringToInt(str.GetData() + 7, idx).Succeeded())
          {
            // now we can lookup the remapped plGameObjectHandle from our array
            const plGameObjectHandle hObject = GoReferences[idx];

            // and stringify the handle into a 'global game object reference', ie. one that contains the internal integer data of the handle
            // a regular runtime world has a reference resolver that is capable to reverse this stringified format to a handle again
            // which will happen once 'InstantiatePrefab' passes the m_Parameters list to the newly created objects
            tmp.SetFormat("#!GGOR-{}", hObject.GetInternalID().m_Data);

            // map local game object reference to global game object reference
            value = tmp.GetData();
          }
        }
      }

      out_parameters.Insert(key, value);
    }
  }
}

void plPrefabReferenceComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hPrefab;

  plPrefabReferenceComponent::SerializePrefabParameters(*GetWorld(), inout_stream, m_Parameters);
}

void plPrefabReferenceComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hPrefab;

  if (uiVersion < 3)
  {
    bool bDummy;
    s >> bDummy;
  }

  plPrefabReferenceComponent::DeserializePrefabParameters(m_Parameters, inout_stream);
}

void plPrefabReferenceComponent::SetPrefabFile(const char* szFile)
{
  plPrefabResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plPrefabResource>(szFile);
    plResourceManager::PreloadResource(hResource);
  }

  SetPrefab(hResource);
}

const char* plPrefabReferenceComponent::GetPrefabFile() const
{
  if (!m_hPrefab.IsValid())
    return "";

  return m_hPrefab.GetResourceID();
}

void plPrefabReferenceComponent::SetPrefab(const plPrefabResourceHandle& hPrefab)
{
  if (m_hPrefab == hPrefab)
    return;

  m_hPrefab = hPrefab;

  if (IsActiveAndInitialized())
  {
    // only add to update list, if not yet activated,
    // since OnActivate will do the instantiation anyway

    GetWorld()->GetComponentManager<plPrefabReferenceComponentManager>()->AddToUpdateList(this);
  }
}

void plPrefabReferenceComponent::InstantiatePrefab()
{
  // now instantiate the prefab
  if (m_hPrefab.IsValid())
  {
    plResourceLock<plPrefabResource> pResource(m_hPrefab, plResourceAcquireMode::AllowLoadingFallback);

    plTransform id;
    id.SetIdentity();

    plPrefabInstantiationOptions options;
    options.m_hParent = GetOwner()->GetHandle();
    options.m_ReplaceNamedRootWithParent = "<Prefab-Root>";
    options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

    // if this ID is valid, this prefab is instantiated at editor runtime
    // replicate the same ID across all instantiated sub components to get correct picking behavior
    if (GetUniqueID() != plInvalidIndex)
    {
      plHybridArray<plGameObject*, 8> createdRootObjects;
      plHybridArray<plGameObject*, 16> createdChildObjects;

      options.m_pCreatedRootObjectsOut = &createdRootObjects;
      options.m_pCreatedChildObjectsOut = &createdChildObjects;

      plUInt32 uiPrevCompCount = GetOwner()->GetComponents().GetCount();

      pResource->InstantiatePrefab(*GetWorld(), id, options, &m_Parameters);

      auto FixComponent = [](plGameObject* pChild, plUInt32 uiUniqueID)
      {
        // while exporting a scene all game objects with this flag are ignored and not exported
        // set this flag on all game objects that were created by instantiating this prefab
        // instead it should be instantiated at runtime again
        // only do this at editor time though, at regular runtime we do want to fully serialize the entire sub tree
        pChild->SetCreatedByPrefab();

        for (auto pComponent : pChild->GetComponents())
        {
          pComponent->SetUniqueID(uiUniqueID);
          pComponent->SetCreatedByPrefab();
        }
      };

      const plUInt32 uiUniqueID = GetUniqueID();

      for (plGameObject* pChild : createdRootObjects)
      {
        if (pChild == GetOwner())
          continue;

        FixComponent(pChild, uiUniqueID);
      }

      for (plGameObject* pChild : createdChildObjects)
      {
        FixComponent(pChild, uiUniqueID);
      }

      for (; uiPrevCompCount < GetOwner()->GetComponents().GetCount(); ++uiPrevCompCount)
      {
        GetOwner()->GetComponents()[uiPrevCompCount]->SetUniqueID(GetUniqueID());
        GetOwner()->GetComponents()[uiPrevCompCount]->SetCreatedByPrefab();
      }
    }
    else
    {
      pResource->InstantiatePrefab(*GetWorld(), id, options, &m_Parameters);
    }
  }
}

void plPrefabReferenceComponent::OnActivated()
{
  SUPER::OnActivated();

  // instantiate the prefab right away, such that game play code can access it as soon as possible
  // additionally the manager may update the instance later on, to properly enable editor work flows
  InstantiatePrefab();
}

void plPrefabReferenceComponent::OnDeactivated()
{
  // if this was created procedurally during editor runtime, we do not need to clear specific nodes
  // after simulation, the scene is deleted anyway

  ClearPreviousInstances();

  SUPER::OnDeactivated();
}

void plPrefabReferenceComponent::ClearPreviousInstances()
{
  if (GetUniqueID() != plInvalidIndex)
  {
    // if this is in the editor, and the 'activate' flag is toggled,
    // get rid of all our created child objects

    plArrayPtr<plComponent* const> comps = GetOwner()->GetComponents();

    for (plUInt32 ip1 = comps.GetCount(); ip1 > 0; ip1--)
    {
      const plUInt32 i = ip1 - 1;

      if (comps[i] != this && // don't try to delete yourself
          comps[i]->WasCreatedByPrefab())
      {
        comps[i]->GetOwningManager()->DeleteComponent(comps[i]);
      }
    }

    for (auto it = GetOwner()->GetChildren(); it.IsValid(); ++it)
    {
      if (it->WasCreatedByPrefab())
      {
        GetWorld()->DeleteObjectNow(it->GetHandle());
      }
    }
  }
}

void plPrefabReferenceComponent::Deinitialize()
{
  if (GetUserFlag(PrefabComponentFlags::SelfDeletion))
  {
    // do nothing, ie do not call OnDeactivated()
    // we do want to keep the created child objects around when this component gets destroyed during simulation
    // that's because the component actually deletes itself when simulation starts
    return;
  }

  // remove the children (through Deactivate)
  OnDeactivated();
}

void plPrefabReferenceComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (GetUniqueID() == plInvalidIndex)
  {
    SetUserFlag(PrefabComponentFlags::SelfDeletion, true);

    // remove the prefab reference component, to prevent issues after another serialization/deserialization
    // and also to save some memory
    DeleteComponent();
  }
}

const plRangeView<const char*, plUInt32> plPrefabReferenceComponent::GetParameters() const
{
  return plRangeView<const char*, plUInt32>([]() -> plUInt32
    { return 0; },
    [this]() -> plUInt32
    { return m_Parameters.GetCount(); },
    [](plUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const plUInt32& uiIt) -> const char*
    { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void plPrefabReferenceComponent::SetParameter(const char* szKey, const plVariant& value)
{
  plHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != plInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;

  if (IsActiveAndInitialized())
  {
    // only add to update list, if not yet activated,
    // since OnActivate will do the instantiation anyway
    GetWorld()->GetComponentManager<plPrefabReferenceComponentManager>()->AddToUpdateList(this);
  }
}

void plPrefabReferenceComponent::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(plTempHashedString(szKey)))
  {
    if (IsActiveAndInitialized())
    {
      // only add to update list, if not yet activated,
      // since OnActivate will do the instantiation anyway
      GetWorld()->GetComponentManager<plPrefabReferenceComponentManager>()->AddToUpdateList(this);
    }
  }
}

bool plPrefabReferenceComponent::GetParameter(const char* szKey, plVariant& out_value) const
{
  plUInt32 it = m_Parameters.Find(szKey);

  if (it == plInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

//////////////////////////////////////////////////////////////////////////

plPrefabReferenceComponentManager::plPrefabReferenceComponentManager(plWorld* pWorld)
  : plComponentManager<ComponentType, plBlockStorageType::Compact>(pWorld)
{
  plResourceManager::GetResourceEvents().AddEventHandler(plMakeDelegate(&plPrefabReferenceComponentManager::ResourceEventHandler, this));
}


plPrefabReferenceComponentManager::~plPrefabReferenceComponentManager()
{
  plResourceManager::GetResourceEvents().RemoveEventHandler(plMakeDelegate(&plPrefabReferenceComponentManager::ResourceEventHandler, this));
}

void plPrefabReferenceComponentManager::Initialize()
{
  auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plPrefabReferenceComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void plPrefabReferenceComponentManager::ResourceEventHandler(const plResourceEvent& e)
{
  if (e.m_Type == plResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<plPrefabResource>())
  {
    plPrefabResourceHandle hPrefab((plPrefabResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hPrefab == hPrefab)
      {
        AddToUpdateList(it);
      }
    }
  }
}

void plPrefabReferenceComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    plPrefabReferenceComponent* pComponent;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    pComponent->m_bInUpdateList = false;
    if (!pComponent->IsActive())
      continue;

    pComponent->ClearPreviousInstances();
    pComponent->InstantiatePrefab();
  }

  m_ComponentsToUpdate.Clear();
}

void plPrefabReferenceComponentManager::AddToUpdateList(plPrefabReferenceComponent* pComponent)
{
  if (!pComponent->m_bInUpdateList)
  {
    m_ComponentsToUpdate.PushBack(pComponent->GetHandle());
    pComponent->m_bInUpdateList = true;
  }
}



PL_STATICLINK_FILE(Core, Core_Prefabs_Implementation_PrefabReferenceComponent);
