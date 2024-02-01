#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/World/World.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <ProcGenPlugin/Components/Implementation/PlacementTile.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>

using namespace plProcGenInternal;

PlacementTile::PlacementTile()
  : m_pOutput(nullptr)

{
}

PlacementTile::PlacementTile(PlacementTile&& other)
{
  m_Desc = other.m_Desc;
  m_pOutput = other.m_pOutput;

  m_State = other.m_State;
  other.m_State = State::Invalid;

  m_PlacedObjects = std::move(other.m_PlacedObjects);
}

PlacementTile::~PlacementTile()
{
  PL_ASSERT_DEV(m_State == State::Invalid, "Implementation error");
}

void PlacementTile::Initialize(const PlacementTileDesc& desc, plSharedPtr<const PlacementOutput>& ref_pOutput)
{
  m_Desc = desc;
  m_pOutput = ref_pOutput;

  m_State = State::Initialized;
}

void PlacementTile::Deinitialize(plWorld& ref_world)
{
  for (auto hObject : m_PlacedObjects)
  {
    ref_world.DeleteObjectDelayed(hObject);
  }
  m_PlacedObjects.Clear();

  m_Desc.m_hComponent.Invalidate();
  m_pOutput = nullptr;
  m_State = State::Invalid;
}

bool PlacementTile::IsValid() const
{
  return !m_Desc.m_hComponent.IsInvalidated() && m_pOutput != nullptr;
}

const PlacementTileDesc& PlacementTile::GetDesc() const
{
  return m_Desc;
}

const PlacementOutput* PlacementTile::GetOutput() const
{
  return m_pOutput;
}

plArrayPtr<const plGameObjectHandle> PlacementTile::GetPlacedObjects() const
{
  return m_PlacedObjects;
}

plBoundingBox PlacementTile::GetBoundingBox() const
{
  return m_Desc.GetBoundingBox();
}

plColor PlacementTile::GetDebugColor() const
{
  switch (m_State)
  {
    case State::Initialized:
      return plColor::Orange;
    case State::Scheduled:
      return plColor::Yellow;
    case State::Finished:
      return plColor::Green;
    default:
      return plColor::DarkRed;
  }
}

void PlacementTile::PreparePlacementData(const plWorld* pWorld, const plPhysicsWorldModuleInterface* pPhysicsModule, PlacementData& ref_placementData)
{
  const plUInt64 uiOutputNameHash = m_pOutput->m_sName.GetHash();
  plUInt32 hashData[] = {
    static_cast<plUInt32>(m_Desc.m_iPosX),
    static_cast<plUInt32>(m_Desc.m_iPosY),
    static_cast<plUInt32>(uiOutputNameHash),
    static_cast<plUInt32>(uiOutputNameHash >> 32),
  };

  ref_placementData.m_pPhysicsModule = pPhysicsModule;
  ref_placementData.m_pWorld = pWorld;
  ref_placementData.m_pOutput = m_pOutput;
  ref_placementData.m_uiTileSeed = plHashingUtils::xxHash32(hashData, sizeof(hashData));
  ref_placementData.m_TileBoundingBox = GetBoundingBox();
  ref_placementData.m_GlobalToLocalBoxTransforms = m_Desc.m_GlobalToLocalBoxTransforms;

  m_State = State::Scheduled;
}

plUInt32 PlacementTile::PlaceObjects(plWorld& ref_world, plArrayPtr<const PlacementTransform> objectTransforms)
{
  PL_PROFILE_SCOPE("PlacementTile::PlaceObjects");

  plGameObjectDesc desc;
  auto& objectsToPlace = m_pOutput->m_ObjectsToPlace;

  plHybridArray<plPrefabResource*, 4> prefabs;
  prefabs.SetCount(objectsToPlace.GetCount());



  for (auto& objectTransform : objectTransforms)
  {
    const plUInt32 uiObjectIndex = objectTransform.m_uiObjectIndex;
    plPrefabResource* pPrefab = prefabs[uiObjectIndex];

    if (pPrefab == nullptr)
    {
      pPrefab = plResourceManager::BeginAcquireResource(objectsToPlace[uiObjectIndex], plResourceAcquireMode::BlockTillLoaded);
      prefabs[uiObjectIndex] = pPrefab;
    }

    plTransform transform = plSimdConversion::ToTransform(objectTransform.m_Transform);
    plHybridArray<plGameObject*, 8> rootObjects;

    plPrefabInstantiationOptions options;
    options.m_pCreatedRootObjectsOut = &rootObjects;

    pPrefab->InstantiatePrefab(ref_world, transform, options);

    // only send the color message, if we actually have a custom color
    if (objectTransform.m_bHasValidColor)
    {
      for (auto pRootObject : rootObjects)
      {
        // Set the color
        plMsgSetColor msg;
        msg.m_Color = objectTransform.m_ObjectColor.ToLinearFloat();
        pRootObject->PostMessageRecursive(msg, plTime::MakeZero(), plObjectMsgQueueType::AfterInitialized);
      }
    }

    for (auto pRootObject : rootObjects)
    {
      m_PlacedObjects.PushBack(pRootObject->GetHandle());
    }
  }

  for (auto pPrefab : prefabs)
  {
    if (pPrefab != nullptr)
    {
      plResourceManager::EndAcquireResource(pPrefab);
    }
  }

  m_State = State::Finished;

  return m_PlacedObjects.GetCount();
}
