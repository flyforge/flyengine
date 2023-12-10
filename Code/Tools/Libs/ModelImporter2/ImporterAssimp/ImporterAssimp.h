#pragma once

#include <ModelImporter2/Importer/Importer.h>

#include <assimp/Importer.hpp>

class plEditableSkeletonJoint;
struct aiNode;
struct aiMesh;

namespace plModelImporter2
{
  class ImporterAssimp : public Importer
  {
  public:
    ImporterAssimp();
    ~ImporterAssimp();

  protected:
    virtual plResult DoImport() override;

  private:
    plResult TraverseAiScene();

    plResult PrepareOutputMesh();
    plResult RecomputeTangents();

    plResult TraverseAiNode(aiNode* pNode, const plMat4& parentTransform, plEditableSkeletonJoint* pCurJoint);
    plResult ProcessAiMesh(aiMesh* pMesh, const plMat4& transform);

    plResult ImportMaterials();
    plResult ImportAnimations();

    plResult ImportBoneColliders(plEditableSkeletonJoint* pJoint);

    Assimp::Importer m_Importer;
    const aiScene* m_pScene = nullptr;
    plUInt32 m_uiTotalMeshVertices = 0;
    plUInt32 m_uiTotalMeshTriangles = 0;

    struct MeshInstance
    {
      plMat4 m_GlobalTransform;
      aiMesh* m_pMesh;
    };

    plMap<plUInt32, plHybridArray<MeshInstance, 4>> m_MeshInstances;
  };

  extern plColor ConvertAssimpType(const aiColor3D& value, bool bInvert = false);
  extern plColor ConvertAssimpType(const aiColor4D& value, bool bInvert = false);
  extern plMat4 ConvertAssimpType(const aiMatrix4x4& value, bool bDummy = false);
  extern plVec3 ConvertAssimpType(const aiVector3D& value, bool bDummy = false);
  extern plQuat ConvertAssimpType(const aiQuaternion& value, bool bDummy = false);
  extern float ConvertAssimpType(float value, bool bDummy = false);
  extern int ConvertAssimpType(int value, bool bDummy = false);

} // namespace plModelImporter2
