#include <JoltPlugin/JoltPluginPCH.h>

#include <Foundation/Types/TagRegistry.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltQueryShapeActorComponent.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Actors/JoltTriggerComponent.h>
#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>
#include <JoltPlugin/Components/JoltSettingsComponent.h>
#include <JoltPlugin/Constraints/JoltConstraintComponent.h>
#include <JoltPlugin/Constraints/JoltFixedConstraintComponent.h>
#include <JoltPlugin/Shapes/JoltShapeBoxComponent.h>
#include <JoltPlugin/System/JoltContacts.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltDebugRenderer.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <Physics/Collision/CollisionCollectorImpl.h>
#include <Physics/Collision/Shape/Shape.h>
#include <RendererCore/Meshes/CustomMeshComponent.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
PL_IMPLEMENT_WORLD_MODULE(plJoltWorldModule);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plJoltWorldModule, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

plCVarBool cvar_JoltSimulationPause("Jolt.Simulation.Pause", false, plCVarFlags::None, "Pauses the physics simulation.");

#ifdef JPH_DEBUG_RENDERER
plCVarBool cvar_JoltDebugDrawConstraints("Jolt.DebugDraw.Constraints", false, plCVarFlags::None, "Visualize physics constraints.");
plCVarBool cvar_JoltDebugDrawConstraintLimits("Jolt.DebugDraw.ConstraintLimits", false, plCVarFlags::None, "Visualize physics constraint limits.");
plCVarBool cvar_JoltDebugDrawConstraintFrames("Jolt.DebugDraw.ConstraintFrames", false, plCVarFlags::None, "Visualize physics constraint frames.");
plCVarBool cvar_JoltDebugDrawBodies("Jolt.DebugDraw.Bodies", false, plCVarFlags::None, "Visualize physics bodies.");
#endif

plCVarBool cvar_JoltVisualizeGeometry("Jolt.Visualize.Geometry", false, plCVarFlags::None, "Renders collision geometry.");
plCVarBool cvar_JoltVisualizeGeometryExclusive("Jolt.Visualize.Exclusive", false, plCVarFlags::Save, "Hides regularly rendered geometry.");
plCVarFloat cvar_JoltVisualizeDistance("Jolt.Visualize.Distance", 30.0f, plCVarFlags::Save, "How far away objects to visualize.");

plJoltWorldModule::plJoltWorldModule(plWorld* pWorld)
  : plPhysicsWorldModuleInterface(pWorld)
//, m_FreeObjectFilterIDs(plJolt::GetSingleton()->GetAllocator()) // could use a proxy allocator to bin those
{
  m_pSimulateTask = PL_DEFAULT_NEW(plDelegateTask<void>, "Jolt::Simulate", plTaskNesting::Never, plMakeDelegate(&plJoltWorldModule::Simulate, this));
  m_pSimulateTask->ConfigureTask("Jolt Simulate", plTaskNesting::Maybe);
}

plJoltWorldModule::~plJoltWorldModule() = default;

class plJoltBodyActivationListener : public JPH::BodyActivationListener
{
public:
  virtual void OnBodyActivated(const JPH::BodyID& bodyID, JPH::uint64 inBodyUserData) override
  {
    const plJoltUserData* pUserData = reinterpret_cast<const plJoltUserData*>(inBodyUserData);
    if (plJoltDynamicActorComponent* pActor = plJoltUserData::GetDynamicActorComponent(pUserData))
    {
      m_pActiveActors->Insert(pActor);
    }

    if (plJoltRagdollComponent* pActor = plJoltUserData::GetRagdollComponent(pUserData))
    {
      (*m_pActiveRagdolls)[pActor]++;
    }

    if (plJoltRopeComponent* pActor = plJoltUserData::GetRopeComponent(pUserData))
    {
      (*m_pActiveRopes)[pActor]++;
    }
  }

  virtual void OnBodyDeactivated(const JPH::BodyID& bodyID, JPH::uint64 inBodyUserData) override
  {
    const plJoltUserData* pUserData = reinterpret_cast<const plJoltUserData*>(inBodyUserData);
    if (plJoltActorComponent* pActor = plJoltUserData::GetActorComponent(pUserData))
    {
      m_pActiveActors->Remove(pActor);
    }

    if (plJoltRagdollComponent* pActor = plJoltUserData::GetRagdollComponent(pUserData))
    {
      if (--(*m_pActiveRagdolls)[pActor] == 0)
      {
        m_pActiveRagdolls->Remove(pActor);
        m_pRagdollsPutToSleep->PushBack(pActor);
      }
    }

    if (plJoltRopeComponent* pActor = plJoltUserData::GetRopeComponent(pUserData))
    {
      if (--(*m_pActiveRopes)[pActor] == 0)
      {
        m_pActiveRopes->Remove(pActor);
      }
    }
  }

  plSet<plJoltDynamicActorComponent*>* m_pActiveActors = nullptr;
  plMap<plJoltRagdollComponent*, plInt32>* m_pActiveRagdolls = nullptr; // value is a ref-count
  plMap<plJoltRopeComponent*, plInt32>* m_pActiveRopes = nullptr;       // value is a ref-count
  plDynamicArray<plJoltRagdollComponent*>* m_pRagdollsPutToSleep = nullptr;
};

class plJoltGroupFilter : public JPH::GroupFilter
{
public:
  virtual bool CanCollide(const JPH::CollisionGroup& group1, const JPH::CollisionGroup& group2) const override
  {
    const plUInt64 id = static_cast<plUInt64>(group1.GetGroupID()) << 32 | group2.GetGroupID();

    return !m_IgnoreCollisions.Contains(id);
  }

  plHashSet<plUInt64> m_IgnoreCollisions;
};

class plJoltGroupFilterIgnoreSame : public JPH::GroupFilter
{
public:
  virtual bool CanCollide(const JPH::CollisionGroup& group1, const JPH::CollisionGroup& group2) const override
  {
    return group1.GetGroupID() != group2.GetGroupID();
  }
};

void plJoltWorldModule::Deinitialize()
{
  m_pSystem = nullptr;
  m_pTempAllocator = nullptr;

  plJoltBodyActivationListener* pActivationListener = reinterpret_cast<plJoltBodyActivationListener*>(m_pActivationListener);
  PL_DEFAULT_DELETE(pActivationListener);
  m_pActivationListener = nullptr;

  plJoltContactListener* pContactListener = reinterpret_cast<plJoltContactListener*>(m_pContactListener);
  PL_DEFAULT_DELETE(pContactListener);
  m_pContactListener = nullptr;

  m_pGroupFilter->Release();
  m_pGroupFilter = nullptr;

  m_pGroupFilterIgnoreSame->Release();
  m_pGroupFilterIgnoreSame = nullptr;
}

class plJoltTempAlloc : public JPH::TempAllocator
{
public:
  plJoltTempAlloc(const char* szName)
    : m_ProxyAlloc(szName, plFoundation::GetAlignedAllocator())
  {
    AddChunk(0);
    m_uiCurChunkIdx = 0;
  }

  ~plJoltTempAlloc()
  {
    for (plUInt32 i = 0; i < m_Chunks.GetCount(); ++i)
    {
      ClearChunk(i);
    }
  }

  virtual void* Allocate(JPH::uint inSize) override
  {
    if (inSize == 0)
      return nullptr;

    const plUInt32 uiNeeded = plMemoryUtils::AlignSize(inSize, 16u);

    while (true)
    {
      const plUInt32 uiRemaining = m_Chunks[m_uiCurChunkIdx].m_uiSize - m_Chunks[m_uiCurChunkIdx].m_uiLastOffset;

      if (uiRemaining >= uiNeeded)
        break;

      AddChunk(uiNeeded);
    }

    auto& lastAlloc = m_Chunks[m_uiCurChunkIdx];

    void* pRes = plMemoryUtils::AddByteOffset(lastAlloc.m_pPtr, lastAlloc.m_uiLastOffset);
    lastAlloc.m_uiLastOffset += uiNeeded;
    return pRes;
  }

  virtual void Free(void* pInAddress, JPH::uint inSize) override
  {
    if (pInAddress == nullptr)
      return;

    const plUInt32 uiAllocSize = plMemoryUtils::AlignSize(inSize, 16u);

    auto& lastAlloc = m_Chunks[m_uiCurChunkIdx];
    lastAlloc.m_uiLastOffset -= uiAllocSize;

    if (lastAlloc.m_uiLastOffset == 0 && m_uiCurChunkIdx > 0)
    {
      // move back to the previous chunk
      --m_uiCurChunkIdx;
    }
  }

  struct Chunk
  {
    void* m_pPtr = nullptr;
    plUInt32 m_uiSize = 0;
    plUInt32 m_uiLastOffset = 0;
  };

  void AddChunk(plUInt32 uiSize)
  {
    ++m_uiCurChunkIdx;

    if (m_uiCurChunkIdx < m_Chunks.GetCount())
      return;

    uiSize = plMath::Max(uiSize, 1024u * 1024u);

    auto& alloc = m_Chunks.ExpandAndGetRef();
    alloc.m_pPtr = PL_NEW_RAW_BUFFER(&m_ProxyAlloc, plUInt8, uiSize);
    alloc.m_uiSize = uiSize;
  }

  void ClearChunk(plUInt32 uiChunkIdx)
  {
    PL_DELETE_RAW_BUFFER(&m_ProxyAlloc, m_Chunks[uiChunkIdx].m_pPtr);
    m_Chunks[uiChunkIdx].m_pPtr = nullptr;
    m_Chunks[uiChunkIdx].m_uiSize = 0;
    m_Chunks[uiChunkIdx].m_uiLastOffset = 0;
  }

  plUInt32 m_uiCurChunkIdx = 0;
  plHybridArray<Chunk, 16> m_Chunks;
  plProxyAllocator m_ProxyAlloc;
};


void plJoltWorldModule::Initialize()
{
  // TODO: it would be better if this were in OnSimulationStarted() to guarantee that the system is always initialized with the latest values
  // however, that doesn't work because plJoltWorldModule is only created by calls to GetOrCreateWorldModule, where Initialize is called, but OnSimulationStarted
  // is queued and executed later

  // ensure the first element is reserved for 'invalid' objects
  m_AllocatedUserData.SetCount(1);

  UpdateSettingsCfg();

  plStringBuilder tmp("Jolt-", GetWorld()->GetName());
  m_pTempAllocator = std::make_unique<plJoltTempAlloc>(tmp);

  const uint32_t cMaxBodies = m_Settings.m_uiMaxBodies;
  const uint32_t cMaxContactConstraints = m_Settings.m_uiMaxBodies * 4;
  const uint32_t cMaxBodyPairs = cMaxContactConstraints * 10;
  const uint32_t cNumBodyMutexes = 0;

  m_pSystem = std::make_unique<JPH::PhysicsSystem>();
  m_pSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, m_ObjectToBroadphase, m_ObjectVsBroadphaseFilter, m_ObjectLayerPairFilter);

  {
    plJoltBodyActivationListener* pListener = PL_DEFAULT_NEW(plJoltBodyActivationListener);
    m_pActivationListener = pListener;
    pListener->m_pActiveActors = &m_ActiveActors;
    pListener->m_pActiveRagdolls = &m_ActiveRagdolls;
    pListener->m_pActiveRopes = &m_ActiveRopes;
    pListener->m_pRagdollsPutToSleep = &m_RagdollsPutToSleep;
    m_pSystem->SetBodyActivationListener(pListener);
  }

  {
    plJoltContactListener* pListener = PL_DEFAULT_NEW(plJoltContactListener);
    pListener->m_pWorld = GetWorld();
    m_pContactListener = pListener;
    m_pSystem->SetContactListener(pListener);
  }

  {
    m_pGroupFilter = new plJoltGroupFilter();
    m_pGroupFilter->AddRef();
  }

  {
    m_pGroupFilterIgnoreSame = new plJoltGroupFilterIgnoreSame();
    m_pGroupFilterIgnoreSame->AddRef();
  }
}

void plJoltWorldModule::OnSimulationStarted()
{
  {
    auto startSimDesc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plJoltWorldModule::StartSimulation, this);
    startSimDesc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    startSimDesc.m_bOnlyUpdateWhenSimulating = true;
    // Start physics simulation as late as possible in the first synchronous phase
    // so all kinematic objects have a chance to update their transform before.
    startSimDesc.m_fPriority = -100000.0f;

    RegisterUpdateFunction(startSimDesc);
  }

  {
    auto fetchResultsDesc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plJoltWorldModule::FetchResults, this);
    fetchResultsDesc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    fetchResultsDesc.m_bOnlyUpdateWhenSimulating = true;
    // Fetch results as early as possible after async phase.
    fetchResultsDesc.m_fPriority = 100000.0f;

    RegisterUpdateFunction(fetchResultsDesc);
  }

  plJoltCollisionFiltering::LoadCollisionFilters();

  UpdateSettingsCfg();
  ApplySettingsCfg();

  m_AccumulatedTimeSinceUpdate = plTime::MakeZero();
}

plUInt32 plJoltWorldModule::CreateObjectFilterID()
{
  if (!m_FreeObjectFilterIDs.IsEmpty())
  {
    plUInt32 uiObjectFilterID = m_FreeObjectFilterIDs.PeekBack();
    m_FreeObjectFilterIDs.PopBack();

    return uiObjectFilterID;
  }

  return m_uiNextObjectFilterID++;
}

void plJoltWorldModule::DeleteObjectFilterID(plUInt32& ref_uiObjectFilterID)
{
  if (ref_uiObjectFilterID == plInvalidIndex)
    return;

  m_FreeObjectFilterIDs.PushBack(ref_uiObjectFilterID);

  ref_uiObjectFilterID = plInvalidIndex;
}

plUInt32 plJoltWorldModule::AllocateUserData(plJoltUserData*& out_pUserData)
{
  if (!m_FreeUserData.IsEmpty())
  {
    plUInt32 uiIndex = m_FreeUserData.PeekBack();
    m_FreeUserData.PopBack();

    out_pUserData = &m_AllocatedUserData[uiIndex];
    return uiIndex;
  }

  out_pUserData = &m_AllocatedUserData.ExpandAndGetRef();
  return m_AllocatedUserData.GetCount() - 1;
}

void plJoltWorldModule::DeallocateUserData(plUInt32& ref_uiUserDataId)
{
  if (ref_uiUserDataId == plInvalidIndex)
    return;

  m_AllocatedUserData[ref_uiUserDataId].Invalidate();

  m_FreeUserDataAfterSimulationStep.PushBack(ref_uiUserDataId);

  ref_uiUserDataId = plInvalidIndex;
}

const plJoltUserData& plJoltWorldModule::GetUserData(plUInt32 uiUserDataId) const
{
  PL_ASSERT_DEBUG(uiUserDataId != plInvalidIndex, "Invalid plJoltUserData ID");

  return m_AllocatedUserData[uiUserDataId];
}

void plJoltWorldModule::SetGravity(const plVec3& vObjectGravity, const plVec3& vCharacterGravity)
{
  m_Settings.m_vObjectGravity = vObjectGravity;
  m_Settings.m_vCharacterGravity = vCharacterGravity;

  if (m_pSystem)
  {
    m_pSystem->SetGravity(plJoltConversionUtils::ToVec3(m_Settings.m_vObjectGravity));
  }
}

plUInt32 plJoltWorldModule::GetCollisionLayerByName(plStringView sName) const
{
  return plJoltCollisionFiltering::GetCollisionFilterConfig().GetFilterGroupByName(sName);
}

void plJoltWorldModule::AddStaticCollisionBox(plGameObject* pObject, plVec3 vBoxSize)
{
  plJoltStaticActorComponent* pActor = nullptr;
  plJoltStaticActorComponent::CreateComponent(pObject, pActor);

  plJoltShapeBoxComponent* pBox;
  plJoltShapeBoxComponent::CreateComponent(pObject, pBox);
  pBox->SetHalfExtents(vBoxSize * 0.5f);
}

void plJoltWorldModule::AddFixedJointComponent(plGameObject* pOwner, const plPhysicsWorldModuleInterface::FixedJointConfig& cfg)
{
  plJoltFixedConstraintComponent* pConstraint = nullptr;
  m_pWorld->GetOrCreateComponentManager<plJoltFixedConstraintComponentManager>()->CreateComponent(pOwner, pConstraint);
  pConstraint->SetActors(cfg.m_hActorA, cfg.m_LocalFrameA, cfg.m_hActorB, cfg.m_LocalFrameB);
}

plUInt32 plJoltWorldModule::QueueBodyToAdd(JPH::Body* pBody, bool bAwake)
{
  if (bAwake)
    m_BodiesToAddAndActivate.PushBack(pBody->GetID().GetIndexAndSequenceNumber());
  else
    m_BodiesToAdd.PushBack(pBody->GetID().GetIndexAndSequenceNumber());

  return m_uiBodiesAddCounter;
}

void plJoltWorldModule::EnableJoinedBodiesCollisions(plUInt32 uiObjectFilterID1, plUInt32 uiObjectFilterID2, bool bEnable)
{
  plJoltGroupFilter* pFilter = static_cast<plJoltGroupFilter*>(m_pGroupFilter);

  const plUInt64 uiMask1 = static_cast<plUInt64>(uiObjectFilterID1) << 32 | uiObjectFilterID2;
  const plUInt64 uiMask2 = static_cast<plUInt64>(uiObjectFilterID2) << 32 | uiObjectFilterID1;

  if (bEnable)
  {
    pFilter->m_IgnoreCollisions.Remove(uiMask1);
    pFilter->m_IgnoreCollisions.Remove(uiMask2);
  }
  else
  {
    pFilter->m_IgnoreCollisions.Insert(uiMask1);
    pFilter->m_IgnoreCollisions.Insert(uiMask2);
  }
}

void plJoltWorldModule::ActivateCharacterController(plJoltCharacterControllerComponent* pCharacter, bool bActivate)
{
  if (bActivate)
  {
    PL_ASSERT_DEBUG(!m_ActiveCharacters.Contains(pCharacter), "plJoltCharacterControllerComponent was activated more than once.");

    m_ActiveCharacters.PushBack(pCharacter);
  }
  else
  {
    if (!m_ActiveCharacters.RemoveAndSwap(pCharacter))
    {
      PL_ASSERT_DEBUG(false, "plJoltCharacterControllerComponent was deactivated more than once.");
    }
  }
}

void plJoltWorldModule::CheckBreakableConstraints()
{
  plWorld* pWorld = GetWorld();

  for (auto it = m_BreakableConstraints.GetIterator(); it.IsValid();)
  {
    plJoltConstraintComponent* pConstraint;
    if (pWorld->TryGetComponent(*it, pConstraint) && pConstraint->IsActive())
    {
      if (pConstraint->ExceededBreakingPoint())
      {
        // notify interested parties, that this constraint is now broken
        plMsgPhysicsJointBroke msg;
        msg.m_hJointObject = pConstraint->GetOwner()->GetHandle();
        pConstraint->GetOwner()->SendEventMessage(msg, pConstraint);

        // currently we don't track the broken state separately, we just remove the component
        pConstraint->GetOwningManager()->DeleteComponent(pConstraint);
        it = m_BreakableConstraints.Remove(it);
      }
      else
      {
        ++it;
      }
    }
    else
    {
      it = m_BreakableConstraints.Remove(it);
    }
  }
}

void plJoltWorldModule::FreeUserDataAfterSimulationStep()
{
  m_FreeUserData.PushBackRange(m_FreeUserDataAfterSimulationStep);
  m_FreeUserDataAfterSimulationStep.Clear();
}

void plJoltWorldModule::StartSimulation(const plWorldModule::UpdateContext& context)
{
  if (cvar_JoltSimulationPause)
    return;

  if (!m_BodiesToAdd.IsEmpty())
  {
    m_uiBodiesAddedSinceOptimize += m_BodiesToAdd.GetCount();

    static_assert(sizeof(JPH::BodyID) == sizeof(plUInt32));

    plUInt32 uiStartIdx = 0;

    while (uiStartIdx < m_BodiesToAdd.GetCount())
    {
      const plUInt32 uiCount = m_BodiesToAdd.GetContiguousRange(uiStartIdx);

      JPH::BodyID* pIDs = reinterpret_cast<JPH::BodyID*>(&m_BodiesToAdd[uiStartIdx]);

      void* pHandle = m_pSystem->GetBodyInterface().AddBodiesPrepare(pIDs, uiCount);
      m_pSystem->GetBodyInterface().AddBodiesFinalize(pIDs, uiCount, pHandle, JPH::EActivation::DontActivate);

      uiStartIdx += uiCount;
    }

    m_BodiesToAdd.Clear();
    ++m_uiBodiesAddCounter;
  }

  if (!m_BodiesToAddAndActivate.IsEmpty())
  {
    m_uiBodiesAddedSinceOptimize += m_BodiesToAddAndActivate.GetCount();

    static_assert(sizeof(JPH::BodyID) == sizeof(plUInt32));

    plUInt32 uiStartIdx = 0;

    while (uiStartIdx < m_BodiesToAddAndActivate.GetCount())
    {
      const plUInt32 uiCount = m_BodiesToAddAndActivate.GetContiguousRange(uiStartIdx);

      JPH::BodyID* pIDs = reinterpret_cast<JPH::BodyID*>(&m_BodiesToAddAndActivate[uiStartIdx]);

      void* pHandle = m_pSystem->GetBodyInterface().AddBodiesPrepare(pIDs, uiCount);
      m_pSystem->GetBodyInterface().AddBodiesFinalize(pIDs, uiCount, pHandle, JPH::EActivation::Activate);

      uiStartIdx += uiCount;
    }

    m_BodiesToAddAndActivate.Clear();
    ++m_uiBodiesAddCounter;
  }

  if (m_uiBodiesAddedSinceOptimize > 128)
  {
    // TODO: not clear whether this could be multi-threaded or done more efficiently somehow
    m_pSystem->OptimizeBroadPhase();
    m_uiBodiesAddedSinceOptimize = 0;
  }

  UpdateSettingsCfg();

  m_SimulatedTimeStep = CalculateUpdateSteps();

  if (m_UpdateSteps.IsEmpty())
    return;

  if (plJoltDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<plJoltDynamicActorComponentManager>())
  {
    pDynamicActorManager->UpdateKinematicActors(m_SimulatedTimeStep);
  }

  if (plJoltQueryShapeActorComponentManager* pQueryShapesManager = GetWorld()->GetComponentManager<plJoltQueryShapeActorComponentManager>())
  {
    pQueryShapesManager->UpdateMovingQueryShapes();
  }

  if (plJoltTriggerComponentManager* pTriggerManager = GetWorld()->GetComponentManager<plJoltTriggerComponentManager>())
  {
    pTriggerManager->UpdateMovingTriggers();
  }

  UpdateConstraints();

  m_SimulateTaskGroupId = plTaskSystem::StartSingleTask(m_pSimulateTask, plTaskPriority::EarlyThisFrame);
}

void plJoltWorldModule::FetchResults(const plWorldModule::UpdateContext& context)
{
  PL_PROFILE_SCOPE("FetchResults");

  {
    PL_PROFILE_SCOPE("Wait for Simulate Task");
    plTaskSystem::WaitForGroup(m_SimulateTaskGroupId);
  }

#ifdef JPH_DEBUG_RENDERER
  if (cvar_JoltDebugDrawConstraints)
    m_pSystem->DrawConstraints(plJoltCore::s_pDebugRenderer.get());

  if (cvar_JoltDebugDrawConstraintLimits)
    m_pSystem->DrawConstraintLimits(plJoltCore::s_pDebugRenderer.get());

  if (cvar_JoltDebugDrawConstraintFrames)
    m_pSystem->DrawConstraintReferenceFrame(plJoltCore::s_pDebugRenderer.get());

  if (cvar_JoltDebugDrawBodies)
  {
    JPH::BodyManager::DrawSettings opt;
    opt.mDrawShape = true;
    opt.mDrawShapeWireframe = true;
    m_pSystem->DrawBodies(opt, plJoltCore::s_pDebugRenderer.get());
  }

  plJoltCore::DebugDraw(GetWorld());
#endif

  // Nothing to fetch if no simulation step was executed
  if (m_UpdateSteps.IsEmpty())
    return;

  if (plJoltDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<plJoltDynamicActorComponentManager>())
  {
    pDynamicActorManager->UpdateDynamicActors();
  }

  for (auto pCharacter : m_ActiveCharacters)
  {
    pCharacter->Update(m_SimulatedTimeStep);
  }

  if (plView* pView = plRenderWorld::GetViewByUsageHint(plCameraUsageHint::MainView, plCameraUsageHint::EditorView, GetWorld()))
  {
    reinterpret_cast<plJoltContactListener*>(m_pContactListener)->m_ContactEvents.m_vMainCameraPosition = pView->GetCamera()->GetPosition();
  }

  reinterpret_cast<plJoltContactListener*>(m_pContactListener)->m_ContactEvents.SpawnPhysicsImpactReactions();
  reinterpret_cast<plJoltContactListener*>(m_pContactListener)->m_ContactEvents.UpdatePhysicsSlideReactions();
  reinterpret_cast<plJoltContactListener*>(m_pContactListener)->m_ContactEvents.UpdatePhysicsRollReactions();

  CheckBreakableConstraints();

  FreeUserDataAfterSimulationStep();

  DebugDrawGeometry();
}

plTime plJoltWorldModule::CalculateUpdateSteps()
{
  plTime tSimulatedTimeStep = plTime::MakeZero();
  m_AccumulatedTimeSinceUpdate += GetWorld()->GetClock().GetTimeDiff();
  m_UpdateSteps.Clear();

  if (m_Settings.m_SteppingMode == plJoltSteppingMode::Variable)
  {
    // always do a single step with the entire time
    m_UpdateSteps.PushBack(m_AccumulatedTimeSinceUpdate);

    tSimulatedTimeStep = m_AccumulatedTimeSinceUpdate;
    m_AccumulatedTimeSinceUpdate = plTime::MakeZero();
  }
  else if (m_Settings.m_SteppingMode == plJoltSteppingMode::Fixed)
  {
    const plTime tFixedStep = plTime::MakeFromSeconds(1.0 / m_Settings.m_fFixedFrameRate);

    plUInt32 uiNumSubSteps = 0;

    while (m_AccumulatedTimeSinceUpdate >= tFixedStep && uiNumSubSteps < m_Settings.m_uiMaxSubSteps)
    {
      m_UpdateSteps.PushBack(tFixedStep);
      ++uiNumSubSteps;

      tSimulatedTimeStep += tFixedStep;
      m_AccumulatedTimeSinceUpdate -= tFixedStep;
    }
  }
  else if (m_Settings.m_SteppingMode == plJoltSteppingMode::SemiFixed)
  {
    plTime tFixedStep = plTime::MakeFromSeconds(1.0 / m_Settings.m_fFixedFrameRate);
    const plTime tMinStep = tFixedStep * 0.25;

    if (tFixedStep * m_Settings.m_uiMaxSubSteps < m_AccumulatedTimeSinceUpdate) // in case too much time has passed
    {
      // if taking N steps isn't sufficient to catch up to the passed time, increase the fixed time step accordingly
      tFixedStep = m_AccumulatedTimeSinceUpdate / (double)m_Settings.m_uiMaxSubSteps;
    }

    while (m_AccumulatedTimeSinceUpdate > tMinStep)
    {
      // prefer fixed time steps
      // but if at the end there is still more than tMinStep time left, do another step with the remaining time
      const plTime tDeltaTime = plMath::Min(tFixedStep, m_AccumulatedTimeSinceUpdate);

      m_UpdateSteps.PushBack(tDeltaTime);

      tSimulatedTimeStep += tDeltaTime;
      m_AccumulatedTimeSinceUpdate -= tDeltaTime;
    }
  }

  return tSimulatedTimeStep;
}

void plJoltWorldModule::Simulate()
{
  if (m_UpdateSteps.IsEmpty())
    return;

  PL_PROFILE_SCOPE("Physics Simulation");

  plTime tDelta = m_UpdateSteps[0];
  plUInt32 uiSteps = 1;

  m_RagdollsPutToSleep.Clear();

  for (plUInt32 i = 1; i < m_UpdateSteps.GetCount(); ++i)
  {
    PL_PROFILE_SCOPE("Physics Sim Step");

    if (m_UpdateSteps[i] == tDelta)
    {
      ++uiSteps;
    }
    else
    {
      // do a single Update call with multiple sub-steps, if possible
      // this saves a bit of time compared to just doing multiple Update calls

      m_pSystem->Update((uiSteps * tDelta).AsFloatInSeconds(), uiSteps, m_pTempAllocator.get(), plJoltCore::GetJoltJobSystem());

      tDelta = m_UpdateSteps[i];
      uiSteps = 1;
    }
  }

  m_pSystem->Update((uiSteps * tDelta).AsFloatInSeconds(), uiSteps, m_pTempAllocator.get(), plJoltCore::GetJoltJobSystem());
}

void plJoltWorldModule::UpdateSettingsCfg()
{
  if (plJoltSettingsComponentManager* pSettingsManager = GetWorld()->GetComponentManager<plJoltSettingsComponentManager>())
  {
    plJoltSettingsComponent* pSettings = pSettingsManager->GetSingletonComponent();

    if (pSettings != nullptr && pSettings->IsModified())
    {
      m_Settings = pSettings->GetSettings();
      pSettings->ResetModified();

      ApplySettingsCfg();
    }
  }
}

void plJoltWorldModule::ApplySettingsCfg()
{
  SetGravity(m_Settings.m_vObjectGravity, m_Settings.m_vCharacterGravity);
}

void plJoltWorldModule::UpdateConstraints()
{
  if (m_RequireUpdate.IsEmpty())
    return;

  plJoltConstraintComponent* pComponent;
  for (auto& hComponent : m_RequireUpdate)
  {
    if (this->m_pWorld->TryGetComponent(hComponent, pComponent))
    {
      pComponent->ApplySettings();
    }
  }

  m_RequireUpdate.Clear();
}

plAtomicInteger32 s_iColMeshVisGeoCounter;

struct DebugVis
{
  const char* m_szMaterial = nullptr;
  plColor m_Color;
};

const char* szMatSolid = "{ e6367876-ddb5-4149-ba80-180af553d463 }";       // Data/Base/Materials/Common/PhysicsColliders.plMaterialAsset
const char* szMatTransparent = "{ ca43dda3-a28c-41fe-ae20-419182e56f87 }"; // Data/Base/Materials/Common/PhysicsCollidersTransparent.plMaterialAsset
const char* szMatTwoSided = "{ b03df0e4-98b7-49ba-8413-d981014a77be }";    // Data/Base/Materials/Common/PhysicsCollidersSoft.plMaterialAsset

static const DebugVis s_Vis[plPhysicsShapeType::Count][2] =
  {
    // Static
    {
      {szMatSolid, plColor::LightSkyBlue}, // non-kinematic
      {szMatSolid, plColor::Red}           // kinematic
    },

    // Dynamic
    {
      {szMatSolid, plColor::Gold},      // non-kinematic
      {szMatSolid, plColor::DodgerBlue} // kinematic
    },

    // Query
    {
      {szMatTransparent, plColor::GreenYellow.WithAlpha(0.5f)}, // non-kinematic
      {szMatTransparent, plColor::GreenYellow.WithAlpha(0.5f)}  // kinematic
    },

    // Trigger
    {
      {szMatTransparent, plColor::Purple.WithAlpha(0.3f)}, // non-kinematic
      {szMatTransparent, plColor::Purple.WithAlpha(0.3f)}  // kinematic
    },

    // Character
    {
      {szMatTransparent, plColor::DarkTurquoise.WithAlpha(0.5f)}, // non-kinematic
      {szMatTransparent, plColor::DarkTurquoise.WithAlpha(0.5f)}  // kinematic
    },

    // Ragdoll
    {
      {szMatSolid, plColor::DeepPink}, // non-kinematic
      {szMatSolid, plColor::DeepPink}  // kinematic
    },

    // Rope
    {
      {szMatSolid, plColor::MediumVioletRed}, // non-kinematic
      {szMatSolid, plColor::MediumVioletRed}  // kinematic
    },

    // Cloth
    {
      {szMatTwoSided, plColor::Crimson}, // non-kinematic
      {szMatTwoSided, plColor::Red}      // kinematic
    },
};

void plJoltWorldModule::DebugDrawGeometry()
{
  plView* pView = plRenderWorld::GetViewByUsageHint(plCameraUsageHint::MainView, plCameraUsageHint::EditorView, GetWorld());

  if (pView == nullptr)
    return;

  ++m_uiDebugGeoLastSeenCounter;

  const plTag& tag = plTagRegistry::GetGlobalRegistry().RegisterTag("PhysicsCollider");

  if (cvar_JoltVisualizeGeometry && cvar_JoltVisualizeGeometryExclusive)
  {
    // deactivate other geometry rendering
    pView->m_IncludeTags.Set(tag);
  }
  else
  {
    pView->m_IncludeTags.Remove(tag);
  }

  if (cvar_JoltVisualizeGeometry)
  {
    const plVec3 vCenterPos = pView->GetCamera()->GetCenterPosition();

    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, plPhysicsShapeType::Static, tag);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, plPhysicsShapeType::Dynamic, tag);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, plPhysicsShapeType::Query, tag);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, plPhysicsShapeType::Ragdoll, tag);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, plPhysicsShapeType::Trigger, tag);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, plPhysicsShapeType::Rope, tag);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, plPhysicsShapeType::Cloth, tag);
  }

  for (auto it = m_DebugDrawComponents.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_uiLastSeenCounter < m_uiDebugGeoLastSeenCounter)
    {
      GetWorld()->DeleteObjectDelayed(it.Value().m_hObject);
      it = m_DebugDrawComponents.Remove(it);
    }
    else
    {
      ++it;
    }
  }

  for (auto it = m_DebugDrawShapeGeo.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_uiLastSeenCounter < m_uiDebugGeoLastSeenCounter)
    {
      it = m_DebugDrawShapeGeo.Remove(it);
    }
    else
    {
      ++it;
    }
  }
}

void plJoltWorldModule::DebugDrawGeometry(const plVec3& vCenter, float fRadius, plPhysicsShapeType::Enum shapeType, const plTag& tag)
{
  const plVec3 vAabbMin = vCenter - plVec3(fRadius);
  const plVec3 vAabbMax = vCenter + plVec3(fRadius);

  JPH::AABox aabb;
  aabb.mMin = plJoltConversionUtils::ToVec3(vAabbMin);
  aabb.mMax = plJoltConversionUtils::ToVec3(vAabbMax);

  JPH::AllHitCollisionCollector<JPH::TransformedShapeCollector> collector;

  plJoltBroadPhaseLayerFilter broadphaseFilter(shapeType);
  JPH::ObjectLayerFilter objectFilterAll;
  JPH::BodyFilter bodyFilterAll;

  m_pSystem->GetNarrowPhaseQuery().CollectTransformedShapes(aabb, collector, broadphaseFilter, objectFilterAll, bodyFilterAll);

  auto* pBodies = &m_pSystem->GetBodyInterface();

  const int cMaxTriangles = 128;


  plStaticArray<plVec3, cMaxTriangles * 3> positionsTmp;
  positionsTmp.SetCountUninitialized(cMaxTriangles * 3);

  plStaticArray<const JPH::PhysicsMaterial*, cMaxTriangles> materialsTmp;
  materialsTmp.SetCountUninitialized(cMaxTriangles);

  plHybridArray<plVec3, cMaxTriangles * 3> positionsTmp2;
  plHybridArray<const JPH::PhysicsMaterial*, cMaxTriangles> materialsTmp2;

  for (const JPH::TransformedShape& ts : collector.mHits)
  {
    DebugBodyShapeKey key;
    key.m_uiBodyID = ts.mBodyID.GetIndexAndSequenceNumber();
    key.m_pShapePtr = ts.mShape.GetPtr();

    bool bExisted = false;
    auto& geo = m_DebugDrawComponents.FindOrAdd(key, &bExisted).Value();
    geo.m_uiLastSeenCounter = m_uiDebugGeoLastSeenCounter;

    DebugGeoShape& shapeGeo = m_DebugDrawShapeGeo[key.m_pShapePtr];
    shapeGeo.m_uiLastSeenCounter = m_uiDebugGeoLastSeenCounter;

    plTransform objTrans;
    objTrans.m_vPosition = plJoltConversionUtils::ToVec3(ts.mShapePositionCOM);
    objTrans.m_qRotation = plJoltConversionUtils::ToQuat(ts.mShapeRotation);
    objTrans.m_vScale = plJoltConversionUtils::ToVec3(ts.mShapeScale);

    if (bExisted)
    {
      plGameObject* pObj;
      if (GetWorld()->TryGetObject(geo.m_hObject, pObj))
      {
        pObj->SetGlobalTransform(objTrans);
      }

      if (!geo.m_bMutableGeometry)
        continue;
    }

    JPH::BodyLockRead lock(m_pSystem->GetBodyLockInterface(), ts.mBodyID);
    if (!lock.Succeeded())
      continue;

    positionsTmp2.Clear();
    materialsTmp2.Clear();

    if (!shapeGeo.m_hMesh.IsValid() || (geo.m_bMutableGeometry && lock.GetBody().IsActive()))
    {
      shapeGeo.m_Bounds = plBoundingBox::MakeInvalid();

      JPH::Shape::GetTrianglesContext ctx;
      ts.mShape->GetTrianglesStart(ctx, JPH::AABox::sBiggest(), JPH::Vec3::sZero(), JPH::Quat::sIdentity(), JPH::Vec3::sReplicate(1.0f));

      while (true)
      {
        const int triCount = ts.mShape->GetTrianglesNext(ctx, cMaxTriangles, reinterpret_cast<JPH::Float3*>(positionsTmp.GetData()), materialsTmp.GetData());

        if (triCount == 0)
          break;

        positionsTmp2.PushBackRange(positionsTmp.GetArrayPtr().GetSubArray(0, triCount * 3));
        materialsTmp2.PushBackRange(materialsTmp.GetArrayPtr().GetSubArray(0, triCount));
      }

      if (positionsTmp2.GetCount() >= 3)
      {
        plDynamicMeshBufferResourceDescriptor desc;
        desc.m_Topology = plGALPrimitiveTopology::Triangles;
        desc.m_uiMaxVertices = positionsTmp2.GetCount();
        desc.m_uiMaxPrimitives = positionsTmp2.GetCount() / 3;
        desc.m_IndexType = plGALIndexType::None;
        desc.m_bColorStream = false;

        plStringBuilder sGuid;
        sGuid.SetFormat("ColMeshVisGeo_{}", s_iColMeshVisGeoCounter.Increment());

        if (!shapeGeo.m_hMesh.IsValid())
        {
          shapeGeo.m_hMesh = plResourceManager::CreateResource<plDynamicMeshBufferResource>(sGuid, std::move(desc));
        }

        plResourceLock<plDynamicMeshBufferResource> pMeshBuf(shapeGeo.m_hMesh, plResourceAcquireMode::BlockTillLoaded);

        plArrayPtr<plDynamicMeshVertex> vertices = pMeshBuf->AccessVertexData();
        for (plUInt32 vtxIdx = 0; vtxIdx < vertices.GetCount(); ++vtxIdx)
        {
          auto& vtx = vertices[vtxIdx];
          vtx.m_vPosition = positionsTmp2[vtxIdx];
          vtx.m_vTexCoord.SetZero();
          vtx.m_vEncodedNormal.SetZero();
          vtx.m_vEncodedTangent.SetZero();

          shapeGeo.m_Bounds.ExpandToInclude(vtx.m_vPosition);
        }
      }
    }

    if (bExisted)
      continue;

    plGameObjectDesc gd;
    gd.m_bDynamic = true;
    gd.m_LocalPosition = objTrans.m_vPosition;
    gd.m_LocalRotation = objTrans.m_qRotation;
    gd.m_LocalScaling = objTrans.m_vScale;

    gd.m_Tags.Set(tag);
    plGameObject* pObj;
    geo.m_hObject = GetWorld()->CreateObject(gd, pObj);
    geo.m_bMutableGeometry = lock.GetBody().IsSoftBody();

    plCustomMeshComponent* pMesh;
    plCustomMeshComponent::CreateComponent(pObj, pMesh);

    const bool bKinematic = (lock.GetBody().GetMotionType() == JPH::EMotionType::Kinematic);
    const auto& vis = s_Vis[plMath::FirstBitLow((plUInt32)shapeType)][bKinematic ? 1 : 0];

    pMesh->SetMeshResource(shapeGeo.m_hMesh);
    pMesh->SetBounds(shapeGeo.m_Bounds);
    pMesh->SetMaterialFile(vis.m_szMaterial);
    pMesh->SetColor(vis.m_Color);
  }
}

PL_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_JoltWorldModule);
