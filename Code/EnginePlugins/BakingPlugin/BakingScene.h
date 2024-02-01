#pragma once

#include <BakingPlugin/Declarations.h>
#include <Core/Graphics/AmbientCubeBasis.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/BakedProbes/BakingInterface.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

class plWorld;
class plProgress;
class plTracerInterface;

class PL_BAKINGPLUGIN_DLL plBakingScene
{
public:
  plResult Extract();

  plResult Bake(const plStringView& sOutputPath, plProgress& progress);

  plResult RenderDebugView(const plMat4& InverseViewProjection, plUInt32 uiWidth, plUInt32 uiHeight, plDynamicArray<plColorGammaUB>& out_Pixels,
    plProgress& progress) const;

public:
  const plWorldGeoExtractionUtil::MeshObjectList& GetMeshObjects() const { return m_MeshObjects; }
  const plBoundingBox& GetBoundingBox() const { return m_BoundingBox; }

  bool IsBaked() const { return m_bIsBaked; }

private:
  friend class plBaking;
  friend class plMemoryUtils;

  plBakingScene();
  ~plBakingScene();

  plBakingSettings m_Settings;
  plDynamicArray<plBakingInternal::Volume, plAlignedAllocatorWrapper> m_Volumes;
  plWorldGeoExtractionUtil::MeshObjectList m_MeshObjects;
  plBoundingBox m_BoundingBox;

  plUInt32 m_uiWorldIndex = plInvalidIndex;
  plUniquePtr<plTracerInterface> m_pTracer;

  bool m_bIsBaked = false;
};

class PL_BAKINGPLUGIN_DLL plBaking : public plBakingInterface
{
  PL_DECLARE_SINGLETON_OF_INTERFACE(plBaking, plBakingInterface);

public:
  plBaking();

  void Startup();
  void Shutdown();

  plBakingScene* GetOrCreateScene(const plWorld& world);
  plBakingScene* GetScene(const plWorld& world);
  const plBakingScene* GetScene(const plWorld& world) const;

  // plBakingInterface
  virtual plResult RenderDebugView(const plWorld& world, const plMat4& InverseViewProjection, plUInt32 uiWidth, plUInt32 uiHeight, plDynamicArray<plColorGammaUB>& out_Pixels, plProgress& progress) const override;
};
