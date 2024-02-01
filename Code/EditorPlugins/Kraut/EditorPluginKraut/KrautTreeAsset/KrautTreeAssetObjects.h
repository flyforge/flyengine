#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct plKrautAssetMaterial
{
  plString m_sLabel;
  plString m_sMaterial;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plKrautAssetMaterial);

class plKrautTreeAssetProperties : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plKrautTreeAssetProperties, plReflectedClass);

public:
  plKrautTreeAssetProperties();
  ~plKrautTreeAssetProperties();

  plString m_sKrautFile;
  float m_fUniformScaling = 1.0f;
  float m_fLodDistanceScale = 1.0f;
  float m_fStaticColliderRadius = 0.4f;
  float m_fTreeStiffness = 10.0f;
  plString m_sSurface;

  plHybridArray<plKrautAssetMaterial, 8> m_Materials;

  plUInt16 m_uiRandomSeedForDisplay = 0;

  plHybridArray<plUInt16, 16> m_GoodRandomSeeds;
};
