#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Mesh/ParticleTypeMesh.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleTypeMeshFactory, 1, plRTTIDefaultAllocator<plParticleTypeMeshFactory>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Mesh", m_sMesh)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    PLASMA_MEMBER_PROPERTY("Material", m_sMaterial)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
    PLASMA_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleTypeMesh, 1, plRTTIDefaultAllocator<plParticleTypeMesh>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const plRTTI* plParticleTypeMeshFactory::GetTypeType() const
{
  return plGetStaticRTTI<plParticleTypeMesh>();
}


void plParticleTypeMeshFactory::CopyTypeProperties(plParticleType* pObject, bool bFirstTime) const
{
  plParticleTypeMesh* pType = static_cast<plParticleTypeMesh*>(pObject);

  pType->m_hMesh.Invalidate();
  pType->m_hMaterial.Invalidate();
  pType->m_sTintColorParameter = plTempHashedString(m_sTintColorParameter.GetData());

  if (!m_sMesh.IsEmpty())
    pType->m_hMesh = plResourceManager::LoadResource<plMeshResource>(m_sMesh);

  if (!m_sMaterial.IsEmpty())
    pType->m_hMaterial = plResourceManager::LoadResource<plMaterialResource>(m_sMaterial);
}

enum class TypeMeshVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added material

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleTypeMeshFactory::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = (int)TypeMeshVersion::Version_Current;
  stream << uiVersion;

  stream << m_sMesh;
  stream << m_sTintColorParameter;

  // Version 2
  stream << m_sMaterial;
}

void plParticleTypeMeshFactory::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  PLASMA_ASSERT_DEV(uiVersion <= (int)TypeMeshVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_sMesh;
  stream >> m_sTintColorParameter;

  if (uiVersion >= 2)
  {
    stream >> m_sMaterial;
  }
}

plParticleTypeMesh::plParticleTypeMesh() = default;
plParticleTypeMesh::~plParticleTypeMesh() = default;

void plParticleTypeMesh::CreateRequiredStreams()
{
  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", plProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", plProcessingStream::DataType::Half4, &m_pStreamColor, false);
  CreateStream("RotationSpeed", plProcessingStream::DataType::Half, &m_pStreamRotationSpeed, false);
  CreateStream("RotationOffset", plProcessingStream::DataType::Half, &m_pStreamRotationOffset, false);
  CreateStream("Axis", plProcessingStream::DataType::Float3, &m_pStreamAxis, true);
}

void plParticleTypeMesh::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  plVec3* pAxis = m_pStreamAxis->GetWritableData<plVec3>();
  plRandom& rng = GetRNG();

  for (plUInt32 i = 0; i < uiNumElements; ++i)
  {
    const plUInt64 uiElementIdx = uiStartIndex + i;

    pAxis[uiElementIdx] = plVec3::MakeRandomDirection(rng);
  }
}

bool plParticleTypeMesh::QueryMeshAndMaterialInfo() const
{
  if (!m_hMesh.IsValid())
  {
    m_bRenderDataCached = true;
    m_hMaterial.Invalidate();
    return true;
  }

  plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::AllowLoadingFallback);
  if (pMesh.GetAcquireResult() != plResourceAcquireResult::Final)
    return false;

  if (!m_hMaterial.IsValid())
  {
    m_hMaterial = pMesh->GetMaterials()[0];

    if (!m_hMaterial.IsValid())
    {
      m_bRenderDataCached = true;
      return true;
    }
  }

  plResourceLock<plMaterialResource> pMaterial(m_hMaterial, plResourceAcquireMode::AllowLoadingFallback);
  if (pMaterial.GetAcquireResult() != plResourceAcquireResult::Final)
    return false;

  m_Bounds = pMesh->GetBounds();

  m_RenderCategory = pMaterial->GetRenderDataCategory();

  m_bRenderDataCached = true;
  return true;
}

void plParticleTypeMesh::ExtractTypeRenderData(plMsgExtractRenderData& msg, const plTransform& instanceTransform) const
{
  if (!m_bRenderDataCached)
  {
    // check if we now know how to render this thing
    if (!QueryMeshAndMaterialInfo())
      return;
  }

  if (!m_hMaterial.IsValid())
    return;

  if (m_RenderCategory.m_uiValue == 0xFFFF)
  {
    m_bRenderDataCached = false;
    return;
  }

  const plUInt32 numParticles = (plUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  PLASMA_PROFILE_SCOPE("PFX: Mesh");

  const plTime tCur = GetOwnerEffect()->GetTotalEffectLifeTime();
  const plColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, plColor::White);

  const plVec4* pPosition = m_pStreamPosition->GetData<plVec4>();
  const plFloat16* pSize = m_pStreamSize->GetData<plFloat16>();
  const plColorLinear16f* pColor = m_pStreamColor->GetData<plColorLinear16f>();
  const plFloat16* pRotationSpeed = m_pStreamRotationSpeed->GetData<plFloat16>();
  const plFloat16* pRotationOffset = m_pStreamRotationOffset->GetData<plFloat16>();
  const plVec3* pAxis = m_pStreamAxis->GetData<plVec3>();

  {
    const plUInt32 uiFlipWinding = 0;

    for (plUInt32 p = 0; p < numParticles; ++p)
    {
      const plUInt32 idx = p;

      plTransform trans;
      trans.m_qRotation = plQuat::MakeFromAxisAndAngle(pAxis[p], plAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[idx]) + pRotationOffset[idx]));
      trans.m_vPosition = pPosition[idx].GetAsVec3();
      trans.m_vScale.Set(pSize[idx]);

      plMeshRenderData* pRenderData = plCreateRenderDataForThisFrame<plMeshRenderData>(nullptr);
      {
        pRenderData->m_GlobalTransform = trans;
        pRenderData->m_GlobalBounds = m_Bounds;
        pRenderData->m_hMesh = m_hMesh;
        pRenderData->m_hMaterial = m_hMaterial;
        pRenderData->m_Color = pColor[idx].ToLinearFloat() * tintColor;

        pRenderData->m_uiSubMeshIndex = 0;
        pRenderData->m_uiUniqueID = 0xFFFFFFFF;

        pRenderData->FillBatchIdAndSortingKey();
      }

      msg.AddRenderData(pRenderData, m_RenderCategory, plRenderData::Caching::Never);
    }
  }
}