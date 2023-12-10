#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgExtractGeometry);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgExtractGeometry, 1, plRTTIDefaultAllocator<plMsgExtractGeometry>)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plWorldGeoExtractionUtil::ExtractWorldGeometry(MeshObjectList& ref_objects, const plWorld& world, ExtractionMode mode, plTagSet* pExcludeTags /*= nullptr*/)
{
  PLASMA_PROFILE_SCOPE("ExtractWorldGeometry");
  PLASMA_LOG_BLOCK("ExtractWorldGeometry", world.GetName());

  plMsgExtractGeometry msg;
  msg.m_Mode = mode;
  msg.m_pMeshObjects = &ref_objects;

  PLASMA_LOCK(world.GetReadMarker());

  for (auto it = world.GetObjects(); it.IsValid(); ++it)
  {
    if (pExcludeTags != nullptr && it->GetTags().IsAnySet(*pExcludeTags))
      continue;

    it->SendMessage(msg);
  }
}

void plWorldGeoExtractionUtil::ExtractWorldGeometry(MeshObjectList& ref_objects, const plWorld& world, ExtractionMode mode, const plDeque<plGameObjectHandle>& selection)
{
  PLASMA_PROFILE_SCOPE("ExtractWorldGeometry");
  PLASMA_LOG_BLOCK("ExtractWorldGeometry", world.GetName());

  plMsgExtractGeometry msg;
  msg.m_Mode = mode;
  msg.m_pMeshObjects = &ref_objects;

  PLASMA_LOCK(world.GetReadMarker());

  for (plGameObjectHandle hObject : selection)
  {
    const plGameObject* pObject;
    if (!world.TryGetObject(hObject, pObject))
      continue;

    pObject->SendMessage(msg);
  }
}

void plWorldGeoExtractionUtil::WriteWorldGeometryToOBJ(const char* szFile, const MeshObjectList& objects, const plMat3& mTransform)
{
  PLASMA_LOG_BLOCK("Write World Geometry to OBJ", szFile);

  plFileWriter file;
  if (file.Open(szFile).Failed())
  {
    plLog::Error("Failed to open file for writing: '{0}'", szFile);
    return;
  }

  plMat4 transform = plMat4::MakeIdentity();
  transform.SetRotationalPart(mTransform);

  plStringBuilder line;

  line = "\n\n# vertices\n\n";
  file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();

  plUInt32 uiVertexOffset = 0;
  plDeque<plUInt32> indices;

  for (const MeshObject& object : objects)
  {
    plResourceLock<plCpuMeshResource> pCpuMesh(object.m_hMeshResource, plResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pCpuMesh.GetAcquireResult() != plResourceAcquireResult::Final)
    {
      continue;
    }

    const auto& meshBufferDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();

    const plVec3* pPositions = nullptr;
    plUInt32 uiElementStride = 0;
    if (plMeshBufferUtils::GetPositionStream(meshBufferDesc, pPositions, uiElementStride).Failed())
    {
      continue;
    }

    plMat4 finalTransform = transform * object.m_GlobalTransform.GetAsMat4();

    // write out all vertices
    for (plUInt32 i = 0; i < meshBufferDesc.GetVertexCount(); ++i)
    {
      const plVec3 pos = finalTransform.TransformPosition(*pPositions);

      line.Format("v {0} {1} {2}\n", plArgF(pos.x, 8), plArgF(pos.y, 8), plArgF(pos.z, 8));
      file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();

      pPositions = plMemoryUtils::AddByteOffset(pPositions, uiElementStride);
    }

    // collect all indices
    bool flip = plGraphicsUtils::IsTriangleFlipRequired(finalTransform.GetRotationalPart());

    if (meshBufferDesc.HasIndexBuffer())
    {
      if (meshBufferDesc.Uses32BitIndices())
      {
        const plUInt32* pTypedIndices = reinterpret_cast<const plUInt32*>(meshBufferDesc.GetIndexBufferData().GetPtr());

        for (plUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
        {
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + 1] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset);
        }
      }
      else
      {
        const plUInt16* pTypedIndices = reinterpret_cast<const plUInt16*>(meshBufferDesc.GetIndexBufferData().GetPtr());

        for (plUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
        {
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + 1] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset);
        }
      }
    }
    else
    {
      for (plUInt32 v = 0; v < meshBufferDesc.GetVertexCount(); ++v)
      {
        indices.PushBack(uiVertexOffset + v);
      }
    }

    uiVertexOffset += meshBufferDesc.GetVertexCount();
  }

  line = "\n\n# triangles\n\n";
  file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();

  for (plUInt32 i = 0; i < indices.GetCount(); i += 3)
  {
    // indices are 1 based in obj
    line.Format("f {0} {1} {2}\n", indices[i + 0] + 1, indices[i + 1] + 1, indices[i + 2] + 1);
    file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();
  }

  plLog::Success("Wrote world geometry to '{0}'", file.GetFilePathAbsolute().GetView());
}

//////////////////////////////////////////////////////////////////////////

void plMsgExtractGeometry::AddMeshObject(const plTransform& transform, plCpuMeshResourceHandle hMeshResource)
{
  m_pMeshObjects->PushBack({transform, hMeshResource});
}

void plMsgExtractGeometry::AddBox(const plTransform& transform, plVec3 vExtents)
{
  const char* szResourceName = "CpuMesh-UnitBox";
  plCpuMeshResourceHandle hBoxMesh = plResourceManager::GetExistingResource<plCpuMeshResource>(szResourceName);
  if (hBoxMesh.IsValid() == false)
  {
    plGeometry geom;
    geom.AddBox(plVec3(1), false);
    geom.TriangulatePolygons();
    geom.ComputeTangents();

    plMeshResourceDescriptor desc;
    desc.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Data/Base/Materials/Common/Pattern.plMaterialAsset

    desc.MeshBufferDesc().AddCommonStreams();
    desc.MeshBufferDesc().AllocateStreamsFromGeometry(geom, plGALPrimitiveTopology::Triangles);

    desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

    desc.ComputeBounds();

    hBoxMesh = plResourceManager::GetOrCreateResource<plCpuMeshResource>(szResourceName, std::move(desc), szResourceName);
  }

  auto& meshObject = m_pMeshObjects->ExpandAndGetRef();
  meshObject.m_GlobalTransform = transform;
  meshObject.m_GlobalTransform.m_vScale *= vExtents;
  meshObject.m_hMeshResource = hBoxMesh;
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Utils_Implementation_WorldGeoExtractionUtil);
