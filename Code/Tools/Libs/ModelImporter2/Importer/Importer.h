#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <ModelImporter2/ModelImporterDLL.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

class plLogInterface;
class plProgress;
class plEditableSkeleton;
class plMeshResourceDescriptor;
struct plAnimationClipResourceDescriptor;

namespace plModelImporter2
{
  struct ImportOptions
  {
    plString m_sSourceFile;

    bool m_bImportSkinningData = false;
    bool m_bRecomputeNormals = false;
    bool m_bRecomputeTangents = false;
    bool m_bNormalizeWeights = false;
    plMat3 m_RootTransform = plMat3::MakeIdentity();

    plMeshResourceDescriptor* m_pMeshOutput = nullptr;
    plEnum<plMeshNormalPrecision> m_MeshNormalsPrecision = plMeshNormalPrecision::Default;
    plEnum<plMeshTexCoordPrecision> m_MeshTexCoordsPrecision = plMeshTexCoordPrecision::Default;
    plEnum<plMeshBoneWeigthPrecision> m_MeshBoneWeightPrecision = plMeshBoneWeigthPrecision::Default;

    plEditableSkeleton* m_pSkeletonOutput = nullptr;

    bool m_bAdditiveAnimation = false;
    plString m_sAnimationToImport; // empty = first in file; "name" = only anim with that name
    plAnimationClipResourceDescriptor* m_pAnimationOutput = nullptr;
    plUInt32 m_uiFirstAnimKeyframe = 0;
    plUInt32 m_uiNumAnimKeyframes = 0;
  };

  enum class PropertySemantic : plInt8
  {
    Unknown = 0,

    DiffuseColor,
    RoughnessValue,
    MetallicValue,
    EmissiveColor,
    TwosidedValue,
  };

  enum class TextureSemantic : plInt8
  {
    Unknown = 0,

    DiffuseMap,
    DiffuseAlphaMap,
    OcclusionMap,
    RoughnessMap,
    MetallicMap,
    OrmMap,
    DisplacementMap,
    NormalMap,
    EmissiveMap,
  };

  struct PLASMA_MODELIMPORTER2_DLL OutputTexture
  {
    plString m_sFilename;
    plString m_sFileFormatExtension;
    plConstByteArrayPtr m_RawData;

    void GenerateFileName(plStringBuilder& out_sName) const;
  };

  struct PLASMA_MODELIMPORTER2_DLL OutputMaterial
  {
    plString m_sName;

    plInt32 m_iReferencedByMesh = -1;                     // if -1, no sub-mesh in the output actually references this
    plMap<TextureSemantic, plString> m_TextureReferences; // semantic -> path
    plMap<PropertySemantic, plVariant> m_Properties;      // semantic -> value
  };

  class PLASMA_MODELIMPORTER2_DLL Importer
  {
  public:
    Importer();
    virtual ~Importer();

    plResult Import(const ImportOptions& options, plLogInterface* pLogInterface = nullptr, plProgress* pProgress = nullptr);
    const ImportOptions& GetImportOptions() const { return m_Options; }

    plMap<plString, OutputTexture> m_OutputTextures; // path -> additional data
    plDeque<OutputMaterial> m_OutputMaterials;
    plDynamicArray<plString> m_OutputAnimationNames;

  protected:
    virtual plResult DoImport() = 0;

    ImportOptions m_Options;
    plProgress* m_pProgress = nullptr;
  };

} // namespace plModelImporter2
