#include <ModelImporter2/ModelImporterPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <ModelImporter2/ImporterMagicaVoxel/ImporterMagicaVoxel.h>

#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Logging/Log.h>


#define OGT_VOX_IMPLEMENTATION
#include <ModelImporter2/ImporterMagicaVoxel/ogt_vox.h>

#define OGT_VOXEL_MESHIFY_IMPLEMENTATION
#include <ModelImporter2/ImporterMagicaVoxel/ogt_voxel_meshify.h>


namespace plModelImporter2
{
  ImporterMagicaVoxel::ImporterMagicaVoxel() = default;
  ImporterMagicaVoxel::~ImporterMagicaVoxel() = default;

  plResult ImporterMagicaVoxel::DoImport()
  {
    const char* szFileName = m_Options.m_sSourceFile;

    plDynamicArray<plUInt8> fileContent;
    fileContent.Reserve(1024 * 1024);

    // Read the whole file into memory since we map BSP data structures directly to memory content
    {
      plFileReader fileReader;

      if (fileReader.Open(szFileName, 1024 * 1024).Failed())
      {
        plLog::Error("Couldn't open '{}' for voxel import.", szFileName);
        return PLASMA_FAILURE;
      }

      plUInt8 Temp[1024 * 4];

      while (plUInt64 uiRead = fileReader.ReadBytes(Temp, PLASMA_ARRAY_SIZE(Temp)))
      {
        fileContent.PushBackRange(plArrayPtr<plUInt8>(Temp, (plUInt32)uiRead));
      }
    }

    if (fileContent.IsEmpty())
    {
      return PLASMA_FAILURE;
    }

    const ogt_vox_scene* scene = ogt_vox_read_scene(fileContent.GetData(), fileContent.GetCount());
    if (!scene)
    {
      plLog::Error("Couldn't open '{}' for voxel import, read_scene failed.", szFileName);
      return PLASMA_FAILURE;
    }

    PLASMA_SCOPE_EXIT(ogt_vox_destroy_scene(scene));

    // Temp storage buffers to build the mesh streams out of
    plDynamicArray<plVec3> positions;
    positions.Reserve(4096);

    plDynamicArray<plVec3> normals;
    normals.Reserve(4096);

    plDynamicArray<plColorGammaUB> colors;
    colors.Reserve(4096);

    plDynamicArray<plUInt32> indices;
    indices.Reserve(8192);

    plUInt32 uiIndexOffset = 0;

    for (uint32_t modelIdx = 0; modelIdx < scene->num_models; ++modelIdx)
    {
      const ogt_vox_model* model = scene->models[modelIdx];

      ogt_voxel_meshify_context ctx;
      memset(&ctx, 0, sizeof(ctx));

      ogt_mesh* mesh = ogt_mesh_from_paletted_voxels_greedy(&ctx, model->voxel_data, model->size_x, model->size_y, model->size_z, (const ogt_mesh_rgba*)&scene->palette.color[0]);
      PLASMA_SCOPE_EXIT(ogt_mesh_destroy(&ctx, mesh));

      if (!mesh)
      {
        plLog::Error("Couldn't generate mesh for voxels in file '{}'.", szFileName);
        return PLASMA_FAILURE;
      }

      ogt_mesh_remove_duplicate_vertices(&ctx, mesh);

      // offset mesh vertices so that the center of the mesh (center of the voxel grid) is at (0,0,0)
      // also apply the root transform in the same go
      {
        const plVec3 originOffset = plVec3(-(float)(model->size_x >> 1), (float)(model->size_z >> 1), (float)(model->size_y >> 1));

        for (uint32_t i = 0; i < mesh->vertex_count; ++i)
        {
          plVec3 pos = plVec3(-mesh->vertices[i].pos.x, mesh->vertices[i].pos.z, mesh->vertices[i].pos.y);
          pos -= originOffset;
          positions.ExpandAndGetRef() = m_Options.m_RootTransform * pos;

          plVec3 norm = plVec3(-mesh->vertices[i].normal.x, mesh->vertices[i].normal.z, mesh->vertices[i].normal.y);
          normals.ExpandAndGetRef() = m_Options.m_RootTransform.TransformDirection(norm);

          colors.ExpandAndGetRef() = plColorGammaUB(mesh->vertices[i].color.r, mesh->vertices[i].color.g, mesh->vertices[i].color.b, mesh->vertices[i].color.a);
        }

        for (uint32_t i = 0; i < mesh->index_count; ++i)
        {
          indices.PushBack(mesh->indices[i] + uiIndexOffset);
        }
      }

      uiIndexOffset += mesh->vertex_count;
    }


    plMeshBufferResourceDescriptor& mb = m_Options.m_pMeshOutput->MeshBufferDesc();

    const plUInt32 uiPosStream = mb.AddStream(plGALVertexAttributeSemantic::Position, plGALResourceFormat::XYZFloat);
    const plUInt32 uiNrmStream = mb.AddStream(plGALVertexAttributeSemantic::Normal, plMeshNormalPrecision::ToResourceFormatNormal(plMeshNormalPrecision::_10Bit));
    const plUInt32 uiColStream = mb.AddStream(plGALVertexAttributeSemantic::Color0, plGALResourceFormat::RGBAUByteNormalized);

    mb.AllocateStreams(positions.GetCount(), plGALPrimitiveTopology::Triangles, indices.GetCount() / 3);

    // Add triangles
    plUInt32 uiFinalTriIdx = 0;
    for (plUInt32 i = 0; i < indices.GetCount(); i += 3, ++uiFinalTriIdx)
    {
      mb.SetTriangleIndices(uiFinalTriIdx, indices[i + 1], indices[i + 0], indices[i + 2]);
    }

    for (plUInt32 i = 0; i < positions.GetCount(); ++i)
    {
      mb.SetVertexData(uiPosStream, i, positions[i]);

      plMeshBufferUtils::EncodeNormal(normals[i], mb.GetVertexData(uiNrmStream, i), plMeshNormalPrecision::_10Bit).IgnoreResult();

      mb.SetVertexData(uiColStream, i, colors[i]);
    }

    m_Options.m_pMeshOutput->SetMaterial(0, plMaterialResource::GetDefaultMaterialFileName(plMaterialResource::DefaultMaterialType::Lit));
    m_Options.m_pMeshOutput->AddSubMesh(indices.GetCount() / 3, 0, 0);
    m_Options.m_pMeshOutput->ComputeBounds();

    return PLASMA_SUCCESS;
  }
} // namespace plModelImporter2
