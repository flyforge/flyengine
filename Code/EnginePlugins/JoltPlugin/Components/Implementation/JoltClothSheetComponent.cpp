#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodySharedSettings.h>
#include <JoltPlugin/Components/JoltClothSheetComponent.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <Physics/SoftBody/SoftBodyMotionProperties.h>
#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plJoltClothSheetRenderData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plJoltClothSheetRenderer, 1, plRTTIDefaultAllocator<plJoltClothSheetRenderer>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_BITFLAGS(plJoltClothSheetFlags, 1)
  PL_ENUM_CONSTANT(plJoltClothSheetFlags::FixedCornerTopLeft),
  PL_ENUM_CONSTANT(plJoltClothSheetFlags::FixedCornerTopRight),
  PL_ENUM_CONSTANT(plJoltClothSheetFlags::FixedCornerBottomRight),
  PL_ENUM_CONSTANT(plJoltClothSheetFlags::FixedCornerBottomLeft),
  PL_ENUM_CONSTANT(plJoltClothSheetFlags::FixedEdgeTop),
  PL_ENUM_CONSTANT(plJoltClothSheetFlags::FixedEdgeRight),
  PL_ENUM_CONSTANT(plJoltClothSheetFlags::FixedEdgeBottom),
  PL_ENUM_CONSTANT(plJoltClothSheetFlags::FixedEdgeLeft),
PL_END_STATIC_REFLECTED_BITFLAGS;

PL_BEGIN_COMPONENT_TYPE(plJoltClothSheetComponent, 2, plComponentMode::Static)
  {
    PL_BEGIN_PROPERTIES
    {
      PL_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new plDefaultValueAttribute(plVec2(0.5f, 0.5f))),
      PL_ACCESSOR_PROPERTY("Segments", GetSegments, SetSegments)->AddAttributes(new plDefaultValueAttribute(plVec2U32(16, 16)), new plClampValueAttribute(plVec2U32(2, 2), plVec2U32(64, 64))),
      PL_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
      PL_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new plDefaultValueAttribute(0.3f), new plClampValueAttribute(0.0f, 10.0f)),
      PL_MEMBER_PROPERTY("GravityFactor", m_fGravityFactor)->AddAttributes(new plDefaultValueAttribute(1.0f)),
      PL_MEMBER_PROPERTY("Damping", m_fDamping)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 1.0f)),
      PL_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new plDefaultValueAttribute(0.05f), new plClampValueAttribute(0.0f, 0.5f)),
      PL_BITFLAGS_ACCESSOR_PROPERTY("Flags", plJoltClothSheetFlags, GetFlags, SetFlags),
      PL_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
      PL_MEMBER_PROPERTY("TextureScale", m_vTextureScale)->AddAttributes(new plDefaultValueAttribute(plVec2(1.0f))),
      PL_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new plDefaultValueAttribute(plColor::White)),
    }
    PL_END_PROPERTIES;
    PL_BEGIN_ATTRIBUTES
    {
      new plCategoryAttribute("Physics/Jolt/Effects"),
    }
    PL_END_ATTRIBUTES;
    PL_BEGIN_MESSAGEHANDLERS
    {
      PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    }
    PL_END_MESSAGEHANDLERS;
  }
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltClothSheetComponent::plJoltClothSheetComponent() = default;
plJoltClothSheetComponent::~plJoltClothSheetComponent() = default;

void plJoltClothSheetComponent::SetSize(plVec2 vVal)
{
  m_vSize = vVal;
  SetupCloth();
}

void plJoltClothSheetComponent::SetSegments(plVec2U32 vVal)
{
  m_vSegments = vVal;
  SetupCloth();
}

void plJoltClothSheetComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vSize;
  s << m_vSegments;
  s << m_uiCollisionLayer;
  s << m_fWindInfluence;
  s << m_fGravityFactor;
  s << m_fDamping;
  s << m_Flags;
  s << m_hMaterial;
  s << m_vTextureScale;
  s << m_Color;
  s << m_fThickness;
}

void plJoltClothSheetComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
   const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_vSize;
  s >> m_vSegments;
  s >> m_uiCollisionLayer;
  s >> m_fWindInfluence;
  s >> m_fGravityFactor;
  s >> m_fDamping;
  s >> m_Flags;
  s >> m_hMaterial;
  s >> m_vTextureScale;
  s >> m_Color;

  if (uiVersion >= 2)
  {
    s >> m_fThickness;
  }
}

void plJoltClothSheetComponent::OnActivated()
{
  SUPER::OnActivated();
}

void plJoltClothSheetComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  SetupCloth();
}

static JPH::Ref<JPH::SoftBodySharedSettings> CreateCloth(plVec2U32 vSegments, plVec2 vSpacing, plBitflags<plJoltClothSheetFlags> flags)
{
  // Create settings
  JPH::SoftBodySharedSettings* settings = new JPH::SoftBodySharedSettings;
  for (plUInt32 y = 0; y < vSegments.y; ++y)
  {
    for (plUInt32 x = 0; x < vSegments.x; ++x)
    {
      JPH::SoftBodySharedSettings::Vertex v;
      v.mPosition = JPH::Float3(x * vSpacing.x, y * vSpacing.y, 0.0f);
      settings->mVertices.push_back(v);
    }
  }

  // Function to get the vertex index of a point on the cloth
  auto GetIdx = [vSegments](plUInt32 x, plUInt32 y) -> plUInt32
  {
    return x + y * vSegments.x;
  };

  if (flags.IsAnyFlagSet())
  {
    if (flags.IsSet(plJoltClothSheetFlags::FixedCornerTopLeft))
    {
      settings->mVertices[GetIdx(0, 0)].mInvMass = 0.0f;
    }

    if (flags.IsSet(plJoltClothSheetFlags::FixedCornerTopRight))
    {
      settings->mVertices[GetIdx(vSegments.x - 1, 0)].mInvMass = 0.0f;
    }

    if (flags.IsSet(plJoltClothSheetFlags::FixedCornerBottomLeft))
    {
      settings->mVertices[GetIdx(0, vSegments.y - 1)].mInvMass = 0.0f;
    }

    if (flags.IsSet(plJoltClothSheetFlags::FixedCornerBottomRight))
    {
      settings->mVertices[GetIdx(vSegments.x - 1, vSegments.y - 1)].mInvMass = 0.0f;
    }

    if (flags.IsSet(plJoltClothSheetFlags::FixedEdgeTop))
    {
      for (plUInt32 x = 0; x < vSegments.x; ++x)
      {
        settings->mVertices[GetIdx(x, 0)].mInvMass = 0.0f;
      }
    }

    if (flags.IsSet(plJoltClothSheetFlags::FixedEdgeBottom))
    {
      for (plUInt32 x = 0; x < vSegments.x; ++x)
      {
        settings->mVertices[GetIdx(x, vSegments.y - 1)].mInvMass = 0.0f;
      }
    }

    if (flags.IsSet(plJoltClothSheetFlags::FixedEdgeLeft))
    {
      for (plUInt32 y = 0; y < vSegments.y; ++y)
      {
        settings->mVertices[GetIdx(0, y)].mInvMass = 0.0f;
      }
    }

    if (flags.IsSet(plJoltClothSheetFlags::FixedEdgeRight))
    {
      for (plUInt32 y = 0; y < vSegments.y; ++y)
      {
        settings->mVertices[GetIdx(vSegments.x - 1, y)].mInvMass = 0.0f;
      }
    }
  }

  // Create edges
  for (plUInt32 y = 0; y < vSegments.y; ++y)
  {
    for (plUInt32 x = 0; x < vSegments.x; ++x)
    {
      JPH::SoftBodySharedSettings::Edge e;
      e.mCompliance = 0.00001f;
      e.mVertex[0] = GetIdx(x, y);
      if (x < vSegments.x - 1)
      {
        e.mVertex[1] = GetIdx(x + 1, y);
        settings->mEdgeConstraints.push_back(e);
      }
      if (y < vSegments.y - 1)
      {
        e.mVertex[1] = GetIdx(x, y + 1);
        settings->mEdgeConstraints.push_back(e);
      }
      if (x < vSegments.x - 1 && y < vSegments.y - 1)
      {
        e.mVertex[1] = GetIdx(x + 1, y + 1);
        settings->mEdgeConstraints.push_back(e);

        e.mVertex[0] = GetIdx(x + 1, y);
        e.mVertex[1] = GetIdx(x, y + 1);
        settings->mEdgeConstraints.push_back(e);
      }
    }
  }

  settings->CalculateEdgeLengths();

  // Create faces
  for (plUInt32 y = 0; y < vSegments.y - 1; ++y)
  {
    for (plUInt32 x = 0; x < vSegments.x - 1; ++x)
    {
      JPH::SoftBodySharedSettings::Face f;
      f.mVertex[0] = GetIdx(x, y);
      f.mVertex[1] = GetIdx(x, y + 1);
      f.mVertex[2] = GetIdx(x + 1, y + 1);
      settings->AddFace(f);

      f.mVertex[1] = GetIdx(x + 1, y + 1);
      f.mVertex[2] = GetIdx(x + 1, y);
      settings->AddFace(f);
    }
  }

  settings->Optimize();

  return settings;
}


void plJoltClothSheetComponent::SetupCloth()
{
  m_BSphere = plBoundingSphere::MakeInvalid();

  if (IsActiveAndSimulating())
  {
    RemoveBody();

    JPH::Ref<JPH::SoftBodySharedSettings> settings = CreateCloth(m_vSegments, m_vSize.CompDiv(plVec2(m_vSegments.x - 1, m_vSegments.y - 1)), m_Flags);

    settings->mVertexRadius = m_fThickness;

    plTransform t = GetOwner()->GetGlobalTransform();

    JPH::SoftBodyCreationSettings cloth(settings, plJoltConversionUtils::ToVec3(t.m_vPosition), plJoltConversionUtils::ToQuat(t.m_qRotation), plJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, plJoltBroadphaseLayer::Cloth));

    cloth.mPressure = 0.0f;
    cloth.mLinearDamping = m_fDamping;
    cloth.mGravityFactor = m_fGravityFactor;
    // cloth.mUserData = TODO ?

    plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
    auto* pSystem = pModule->GetJoltSystem();
    auto* pBodies = &pSystem->GetBodyInterface();

    auto pBody = pBodies->CreateSoftBody(cloth);

    m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

    pModule->QueueBodyToAdd(pBody, true);
  }

  TriggerLocalBoundsUpdate();
}

void plJoltClothSheetComponent::RemoveBody()
{
  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (!bodyId.IsInvalid())
  {
    auto* pSystem = pModule->GetJoltSystem();
    auto* pBodies = &pSystem->GetBodyInterface();

    if (pBodies->IsAdded(bodyId))
    {
      pBodies->RemoveBody(bodyId);
    }

    pBodies->DestroyBody(bodyId);
    m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
  }

  // TODO: currently not yet needed
  // pModule->DeallocateUserData(m_uiUserDataIndex);
  // pModule->DeleteObjectFilterID(m_uiObjectFilterID);
}

void plJoltClothSheetComponent::UpdateBodyBounds()
{
  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();
  const JPH::BodyLockInterface* pLi = &pSystem->GetBodyLockInterface();

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (bodyId.IsInvalid())
    return;

  // Get write access to the body
  JPH::BodyLockRead lock(*pLi, bodyId);
  if (!lock.SucceededAndIsInBroadPhase())
    return;

  plBoundingSphere prevBounds = m_BSphere;

  const JPH::Body& body = lock.GetBody();

  // TODO: should rather iterate over all active (soft) bodies, than to check this here
  if (!body.IsActive())
    return;

  const JPH::AABox box = body.GetWorldSpaceBounds();

  const plTransform t = GetOwner()->GetGlobalTransform().GetInverse();

  m_BSphere.m_vCenter = t.TransformPosition(plJoltConversionUtils::ToVec3(box.GetCenter()));

  const plVec3 ext = plJoltConversionUtils::ToVec3(box.GetExtent());
  m_BSphere.m_fRadius = plMath::Max(ext.x, ext.y, ext.z);

  if (prevBounds != m_BSphere)
  {
    SetUserFlag(0, true);
  }
}

void plJoltClothSheetComponent::OnDeactivated()
{
  RemoveBody();

  SUPER::OnDeactivated();
}

plResult plJoltClothSheetComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  if (m_BSphere.IsValid())
  {
    ref_bounds.ExpandToInclude(plBoundingBoxSphere::MakeFromSphere(m_BSphere));
  }
  else
  {
    plBoundingBox box = plBoundingBox::MakeInvalid();
    box.ExpandToInclude(plVec3::MakeZero());
    box.ExpandToInclude(plVec3(0, 0, -0.1f));
    box.ExpandToInclude(plVec3(m_vSize.x, m_vSize.y, 0.1f));

    ref_bounds.ExpandToInclude(plBoundingBoxSphere::MakeFromBox(box));
  }

  return PL_SUCCESS;
}

void plJoltClothSheetComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  auto pRenderData = plCreateRenderDataForThisFrame<plJoltClothSheetRenderData>(GetOwner());
  pRenderData->m_uiUniqueID = GetUniqueIdForRendering();
  pRenderData->m_Color = m_Color;
  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_uiBatchId = plHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash());
  pRenderData->m_uiSortingKey = pRenderData->m_uiBatchId;
  pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
  pRenderData->m_hMaterial = m_hMaterial;
  pRenderData->m_vTextureScale = m_vTextureScale;

  if (!IsActiveAndSimulating())
  {
    pRenderData->m_uiVerticesX = 2;
    pRenderData->m_uiVerticesY = 2;

    pRenderData->m_Positions = PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plVec3, 4);
    pRenderData->m_Indices = PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plUInt16, 6);

    pRenderData->m_Positions[0] = plVec3(0, 0, 0);
    pRenderData->m_Positions[1] = plVec3(m_vSize.x, 0, 0);
    pRenderData->m_Positions[2] = plVec3(0, m_vSize.y, 0);
    pRenderData->m_Positions[3] = plVec3(m_vSize.x, m_vSize.y, 0);

    pRenderData->m_Indices[0] = 0;
    pRenderData->m_Indices[1] = 1;
    pRenderData->m_Indices[2] = 2;

    pRenderData->m_Indices[3] = 1;
    pRenderData->m_Indices[4] = 3;
    pRenderData->m_Indices[5] = 2;
  }
  else
  {
    pRenderData->m_uiVerticesX = m_vSegments.x;
    pRenderData->m_uiVerticesY = m_vSegments.y;

    const plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();
    auto* pSystem = pModule->GetJoltSystem();
    const JPH::BodyLockInterface* pLi = &pSystem->GetBodyLockInterface();

    JPH::BodyID bodyId(m_uiJoltBodyID);

    if (bodyId.IsInvalid())
      return;

    // Get the body
    JPH::BodyLockRead lock(*pLi, bodyId);
    if (!lock.SucceededAndIsInBroadPhase())
      return;

    pRenderData->m_Positions = PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plVec3, pRenderData->m_uiVerticesX * pRenderData->m_uiVerticesY);
    pRenderData->m_Indices = PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plUInt16, (pRenderData->m_uiVerticesX - 1) * (pRenderData->m_uiVerticesY - 1) * 2 * 3);

    const JPH::Body& body = lock.GetBody();
    const JPH::SoftBodyMotionProperties* pMotion = static_cast<const JPH::SoftBodyMotionProperties*>(body.GetMotionProperties());

    const auto& transformed_shape = body.GetTransformedShape();

    // Vec3 scale = transformed_shape.GetShapeScale();
    // RMat44 matrix = transformed_shape.GetCenterOfMassTransform().PreScaled(scale);

    pRenderData->m_GlobalTransform.m_vPosition = plJoltConversionUtils::ToVec3(transformed_shape.GetCenterOfMassTransform().GetTranslation());
    pRenderData->m_GlobalTransform.m_qRotation = plJoltConversionUtils::ToQuat(transformed_shape.GetCenterOfMassTransform().GetRotation().GetQuaternion());

    const JPH::Array<JPH::SoftBodyMotionProperties::Vertex>& particles = pMotion->GetVertices();

    // copy over the vertex positions
    {
      plUInt32 vidx = 0;
      for (plUInt32 y = 0; y < pRenderData->m_uiVerticesY; ++y)
      {
        for (plUInt32 x = 0; x < pRenderData->m_uiVerticesX; ++x, ++vidx)
        {
          pRenderData->m_Positions[vidx] = plJoltConversionUtils::ToVec3(particles[vidx].mPosition);
        }
      }
    }

    // create the triangle indices
    {
      plUInt32 tidx = 0;
      plUInt16 vidx = 0;
      for (plUInt16 y = 0; y < pRenderData->m_uiVerticesY - 1; ++y)
      {
        for (plUInt16 x = 0; x < pRenderData->m_uiVerticesX - 1; ++x, ++vidx)
        {
          pRenderData->m_Indices[tidx++] = vidx;
          pRenderData->m_Indices[tidx++] = vidx + 1;
          pRenderData->m_Indices[tidx++] = vidx + pRenderData->m_uiVerticesX;

          pRenderData->m_Indices[tidx++] = vidx + 1;
          pRenderData->m_Indices[tidx++] = vidx + pRenderData->m_uiVerticesX + 1;
          pRenderData->m_Indices[tidx++] = vidx + pRenderData->m_uiVerticesX;
        }

        ++vidx;
      }
    }
  }

  plRenderData::Category category = m_RenderDataCategory;

  if (!category.IsValid())
  {
    category = plDefaultRenderDataCategories::LitOpaque; // use as default fallback

    if (m_hMaterial.IsValid())
    {
      plResourceLock<plMaterialResource> pMaterial(m_hMaterial, plResourceAcquireMode::AllowLoadingFallback);

      if (pMaterial.GetAcquireResult() != plResourceAcquireResult::LoadingFallback)
      {
        // if this is the final result, cache it
        m_RenderDataCategory = pMaterial->GetRenderDataCategory();
      }

      category = pMaterial->GetRenderDataCategory();
    }
  }

  msg.AddRenderData(pRenderData, category, plRenderData::Caching::Never);
}

void plJoltClothSheetComponent::SetFlags(plBitflags<plJoltClothSheetFlags> flags)
{
  m_Flags = flags;
  SetupCloth();
}

void plJoltClothSheetComponent::SetMaterialFile(const char* szFile)
{
  plMaterialResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plMaterialResource>(szFile);
  }

  m_hMaterial = hResource;
}

const char* plJoltClothSheetComponent::GetMaterialFile() const
{
  if (m_hMaterial.IsValid())
    return m_hMaterial.GetResourceID();

  return "";
}

void plJoltClothSheetComponent::Update()
{
  if (!IsActiveAndSimulating())
    return;

  // TODO: only do this every once in a while
  UpdateBodyBounds();

  if (GetOwner()->GetVisibilityState(60) == plVisibilityState::Direct)
  {
    // only apply wind to directly visible pieces of cloth
    ApplyWind();
  }
}


void plJoltClothSheetComponent::ApplyWind()
{
  if (m_fWindInfluence <= 0.0f)
    return;

  if (!m_BSphere.IsValid())
    return;

  if (const plWindWorldModuleInterface* pWind = GetWorld()->GetModuleReadOnly<plWindWorldModuleInterface>())
  {
    const plVec3 vSamplePos = GetOwner()->GetGlobalTransform().TransformPosition(m_BSphere.m_vCenter);

    const plVec3 vWind = pWind->GetWindAt(vSamplePos) * m_fWindInfluence;

    if (!vWind.IsZero())
    {
      plVec3 windForce = vWind;
      windForce += pWind->ComputeWindFlutter(vWind, vWind.GetOrthogonalVector(), 5.0f, GetOwner()->GetStableRandomSeed());

      JPH::Vec3 windVel = plJoltConversionUtils::ToVec3(windForce);

      plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();
      auto* pSystem = pModule->GetJoltSystem();
      const JPH::BodyLockInterface* pLi = &pSystem->GetBodyLockInterface();

      JPH::BodyID bodyId(m_uiJoltBodyID);

      if (bodyId.IsInvalid())
        return;

      // Get write access to the body
      JPH::BodyLockWrite lock(*pLi, bodyId);
      if (!lock.SucceededAndIsInBroadPhase())
        return;

      JPH::Body& body = lock.GetBody();
      JPH::SoftBodyMotionProperties* pMotion = static_cast<JPH::SoftBodyMotionProperties*>(body.GetMotionProperties());

      if (!body.IsActive())
      {
        pSystem->GetBodyInterfaceNoLock().ActivateBody(bodyId);
      }

      JPH::Array<JPH::SoftBodyMotionProperties::Vertex>& particles = pMotion->GetVertices();

      // randomize which vertices get the wind velocity applied,
      // both to save performance and also to introduce a nice ripple effect
      const plUInt32 uiStart = GetWorld()->GetRandomNumberGenerator().UIntInRange(plMath::Min<plUInt32>(16u, particles.size()));
      const plUInt32 uiStep = GetWorld()->GetRandomNumberGenerator().IntInRange(16, 32);

      for (plUInt32 i = uiStart; i < particles.size(); i += uiStep)
      {
        if (particles[i].mInvMass > 0)
        {
          particles[i].mVelocity = windVel;
        }
      }
    }
  }
}

plJoltClothSheetRenderer::plJoltClothSheetRenderer()
{
  CreateVertexBuffer();
}

plJoltClothSheetRenderer::~plJoltClothSheetRenderer() = default;

void plJoltClothSheetRenderer::GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(plDefaultRenderDataCategories::LitOpaque);
  ref_categories.PushBack(plDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(plDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(plDefaultRenderDataCategories::Selection);
}

void plJoltClothSheetRenderer::GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(plGetStaticRTTI<plJoltClothSheetRenderData>());
}

void plJoltClothSheetRenderer::RenderBatch(const plRenderViewContext& renderViewContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const
{
  const bool bNeedsNormals = (renderViewContext.m_pViewData->m_CameraUsageHint != plCameraUsageHint::Shadow);


  plRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  plGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  plInstanceData* pInstanceData = pPass->GetPipeline()->GetFrameDataProvider<plInstanceDataProvider>()->GetData(renderViewContext);
  pInstanceData->BindResources(pRenderContext);

  pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");

  plResourceLock<plDynamicMeshBufferResource> pBuffer(m_hDynamicMeshBuffer, plResourceAcquireMode::BlockTillLoaded);

  for (auto it = batch.GetIterator<plJoltClothSheetRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const plJoltClothSheetRenderData* pRenderData = it;

    PL_ASSERT_DEV(pRenderData->m_uiVerticesX > 1 && pRenderData->m_uiVerticesY > 1, "Invalid cloth render data");

    pRenderContext->BindMaterial(pRenderData->m_hMaterial);

    plUInt32 uiInstanceDataOffset = 0;
    plArrayPtr<plPerInstanceData> instanceData = pInstanceData->GetInstanceData(1, uiInstanceDataOffset);

    instanceData[0].ObjectToWorld = pRenderData->m_GlobalTransform;
    instanceData[0].ObjectToWorldNormal = instanceData[0].ObjectToWorld;
    instanceData[0].GameObjectID = pRenderData->m_uiUniqueID;
    instanceData[0].Color = pRenderData->m_Color;

    pInstanceData->UpdateInstanceData(pRenderContext, 1);

    {
      auto pVertexData = pBuffer->AccessVertexData();
      auto pIndexData = pBuffer->AccessIndex16Data();

      const float fDivU = 1.0f / (pRenderData->m_uiVerticesX - 1);
      const float fDivY = 1.0f / (pRenderData->m_uiVerticesY - 1);

      const plUInt16 width = pRenderData->m_uiVerticesX;

      if (bNeedsNormals)
      {
        const plUInt16 widthM1 = width - 1;
        const plUInt16 heightM1 = pRenderData->m_uiVerticesY - 1;

        plUInt16 topIdx = 0;

        plUInt32 vidx = 0;
        for (plUInt16 y = 0; y < pRenderData->m_uiVerticesY; ++y)
        {
          plUInt16 leftIdx = 0;
          const plUInt16 bottomIdx = plMath::Min<plUInt16>(y + 1, heightM1);

          const plUInt32 yOff = y * width;
          const plUInt32 yOffTop = topIdx * width;
          const plUInt32 yOffBottom = bottomIdx * width;

          for (plUInt16 x = 0; x < width; ++x, ++vidx)
          {
            const plUInt16 rightIdx = plMath::Min<plUInt16>(x + 1, widthM1);

            const plVec3 leftPos = pRenderData->m_Positions[yOff + leftIdx];
            const plVec3 rightPos = pRenderData->m_Positions[yOff + rightIdx];
            const plVec3 topPos = pRenderData->m_Positions[yOffTop + x];
            const plVec3 bottomPos = pRenderData->m_Positions[yOffBottom + x];

            const plVec3 leftToRight = rightPos - leftPos;
            const plVec3 bottomToTop = topPos - bottomPos;
            plVec3 normal = -leftToRight.CrossRH(bottomToTop);
            normal.NormalizeIfNotZero(plVec3(0, 0, 1)).IgnoreResult();

            plVec3 tangent = leftToRight;
            tangent.NormalizeIfNotZero(plVec3(1, 0, 0)).IgnoreResult();

            pVertexData[vidx].m_vPosition = pRenderData->m_Positions[vidx];
            pVertexData[vidx].m_vTexCoord = plVec2(x * fDivU, y * fDivY).CompMul(pRenderData->m_vTextureScale);
            pVertexData[vidx].EncodeNormal(normal);
            pVertexData[vidx].EncodeTangent(tangent, 1.0f);

            leftIdx = x;
          }

          topIdx = y;
        }
      }
      else
      {
        plUInt32 vidx = 0;
        for (plUInt16 y = 0; y < pRenderData->m_uiVerticesY; ++y)
        {
          for (plUInt16 x = 0; x < width; ++x, ++vidx)
          {
            pVertexData[vidx].m_vPosition = pRenderData->m_Positions[vidx];
            pVertexData[vidx].m_vTexCoord = plVec2(x * fDivU, y * fDivY).CompMul(pRenderData->m_vTextureScale);
            pVertexData[vidx].EncodeNormal(plVec3::MakeAxisZ());
            pVertexData[vidx].EncodeTangent(plVec3::MakeAxisX(), 1.0f);
          }
        }
      }

      plMemoryUtils::Copy<plUInt16>(pIndexData.GetPtr(), pRenderData->m_Indices.GetPtr(), pRenderData->m_Indices.GetCount());
    }

    const plUInt32 uiNumPrimitives = (pRenderData->m_uiVerticesX - 1) * (pRenderData->m_uiVerticesY - 1) * 2;

    pBuffer->UpdateGpuBuffer(pGALCommandEncoder, 0, pRenderData->m_uiVerticesX * pRenderData->m_uiVerticesY);

    // redo this after the primitive count has changed
    pRenderContext->BindMeshBuffer(m_hDynamicMeshBuffer);

    renderViewContext.m_pRenderContext->DrawMeshBuffer(uiNumPrimitives).IgnoreResult();
  }
}

void plJoltClothSheetRenderer::CreateVertexBuffer()
{
  if (m_hDynamicMeshBuffer.IsValid())
    return;

  m_hDynamicMeshBuffer = plResourceManager::GetExistingResource<plDynamicMeshBufferResource>("JoltClothSheet");

  if (!m_hDynamicMeshBuffer.IsValid())
  {
    const plUInt32 uiMaxVerts = 64;

    plDynamicMeshBufferResourceDescriptor desc;
    desc.m_uiMaxVertices = uiMaxVerts * uiMaxVerts;
    desc.m_IndexType = plGALIndexType::UShort;
    desc.m_uiMaxPrimitives = plMath::Square(uiMaxVerts - 1) * 2;

    m_hDynamicMeshBuffer = plResourceManager::GetOrCreateResource<plDynamicMeshBufferResource>("JoltClothSheet", std::move(desc), "Jolt Cloth Sheet Buffer");
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plJoltClothSheetComponentManager::plJoltClothSheetComponentManager(plWorld* pWorld)
  : plComponentManager(pWorld)
{
}

plJoltClothSheetComponentManager::~plJoltClothSheetComponentManager() = default;

void plJoltClothSheetComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plJoltClothSheetComponentManager::Update, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plJoltClothSheetComponentManager::UpdateBounds, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void plJoltClothSheetComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->Update();
    }
  }
}

void plJoltClothSheetComponentManager::UpdateBounds(const plWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUserFlag(0))
    {
      it->TriggerLocalBoundsUpdate();

      // reset update bounds flag
      it->SetUserFlag(0, false);
    }
  }
}
