#pragma once

#include <Core/ResourceManager/Resource.h>
#include <KrautPlugin/KrautDeclarations.h>

#include <KrautGenerator/Description/LodDesc.h>
#include <KrautGenerator/Description/TreeStructureDesc.h>

struct plKrautTreeResourceDescriptor;

namespace Kraut
{
  struct TreeStructure;
  struct TreeStructureDesc;
}; // namespace Kraut

using plKrautGeneratorResourceHandle = plTypedResourceHandle<class plKrautGeneratorResource>;
using plKrautTreeResourceHandle = plTypedResourceHandle<class plKrautTreeResource>;
using plMaterialResourceHandle = plTypedResourceHandle<class plMaterialResource>;

struct plKrautMaterialDescriptor
{
  plKrautMaterialType m_MaterialType = plKrautMaterialType::None;
  plKrautBranchType m_BranchType = plKrautBranchType::None;
  plMaterialResourceHandle m_hMaterial;
};

struct PL_KRAUTPLUGIN_DLL plKrautGeneratorResourceDescriptor : public plRefCounted
{
  Kraut::TreeStructureDesc m_TreeStructureDesc;
  Kraut::LodDesc m_LodDesc[5];

  plHybridArray<plKrautMaterialDescriptor, 4> m_Materials;

  plString m_sSurfaceResource;
  float m_fStaticColliderRadius = 0.5f;
  float m_fUniformScaling = 1.0f;
  float m_fLodDistanceScale = 1.0f;
  float m_fTreeStiffness = 10.0f;

  plUInt16 m_uiDefaultDisplaySeed = 0;
  plHybridArray<plUInt16, 16> m_GoodRandomSeeds;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);
};

class PL_KRAUTPLUGIN_DLL plKrautGeneratorResource : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plKrautGeneratorResource, plResource);
  PL_RESOURCE_DECLARE_COMMON_CODE(plKrautGeneratorResource);

public:
  plKrautGeneratorResource();

  const plSharedPtr<plKrautGeneratorResourceDescriptor>& GetDescriptor() const
  {
    return m_GeneratorDesc;
  }

  plKrautTreeResourceHandle GenerateTree(const plSharedPtr<plKrautGeneratorResourceDescriptor>& descriptor, plUInt32 uiRandomSeed) const;
  plKrautTreeResourceHandle GenerateTreeWithGoodSeed(const plSharedPtr<plKrautGeneratorResourceDescriptor>& descriptor, plUInt16 uiGoodSeedIndex) const;

  void GenerateTreeDescriptor(const plSharedPtr<plKrautGeneratorResourceDescriptor>& descriptor, plKrautTreeResourceDescriptor& dstDesc, plUInt32 uiRandomSeed) const;


private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plMutex m_DataMutex;
  plSharedPtr<plKrautGeneratorResourceDescriptor> m_GeneratorDesc;


  struct BranchNodeExtraData
  {
    float m_fSegmentLength = 0.0f;
    float m_fDistanceAlongBranch = 0.0f;
    float m_fBendinessAlongBranch = 0.0f;
  };

  struct BranchExtraData
  {
    plInt32 m_iParentBranch = -1;        // trunks have parent ID -1
    plUInt16 m_uiParentBranchNodeID = 0; // at which node of the parent, this branch is attached
    plUInt8 m_uiBranchLevel = 0;
    plDynamicArray<BranchNodeExtraData> m_Nodes;
    float m_fDistanceToAnchor = 0; // this will be zero for level 0 (trunk) and 1 (main branches) and only > 0 starting at level 2 (twigs)
    float m_fBendinessToAnchor = 0;
    plUInt32 m_uiRandomNumber = 0;
  };

  struct TreeStructureExtraData
  {
    plDynamicArray<BranchExtraData> m_Branches;
  };

  mutable plKrautTreeResourceHandle m_hFallbackResource;

  void InitializeExtraData(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure, plUInt32 uiRandomSeed) const;
  void ComputeDistancesAlongBranches(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const;
  void ComputeDistancesToAnchors(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const;
  void ComputeBendinessAlongBranches(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure, float fWoodBendiness, float fTwigBendiness) const;
  void ComputeBendinessToAnchors(TreeStructureExtraData& extraData, const Kraut::TreeStructure& treeStructure) const;
  void GenerateExtraData(TreeStructureExtraData& treeStructureExtraData, const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::TreeStructure& treeStructure, plUInt32 uiRandomSeed, float fWoodBendiness, float fTwigBendiness) const;
};
