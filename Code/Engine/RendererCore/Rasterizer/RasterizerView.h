#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/ArrayPtr.h>
#include <RendererCore/RendererCoreDLL.h>

class Rasterizer;
class plRasterizerObject;
class plColorLinearUB;
class plCamera;
class plSimdBBox;

class PLASMA_RENDERERCORE_DLL plRasterizerView final
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plRasterizerView);

public:
  plRasterizerView();
  ~plRasterizerView();

  /// \brief Changes the resolution of the view. Has to be called at least once before starting to render anything.
  void SetResolution(plUInt32 uiWidth, plUInt32 uiHeight, float fAspectRatio);

  plUInt32 GetResolutionX() const { return m_uiResolutionX; }
  plUInt32 GetResolutionY() const { return m_uiResolutionY; }

  /// \brief Prepares the view to rasterize a new scene.
  void BeginScene();

  /// \brief Finishes rasterizing the scene. Visibility queries only work after this.
  void EndScene();

  /// \brief Writes an RGBA8 representation of the depth values to targetBuffer.
  ///
  /// The buffer must be large enough for the chosen resolution.
  void ReadBackFrame(plArrayPtr<plColorLinearUB> targetBuffer) const;

  /// \brief Sets the camera from which to extract the rendering position, direction and field-of-view.
  void SetCamera(const plCamera* pCamera)
  {
    m_pCamera = pCamera;
  }

  /// \brief Adds an object as an occluder to the scene. Once all occluders have been rasterized, visibility queries can be done.
  void AddObject(const plRasterizerObject* pObject, const plTransform& transform)
  {
    auto& inst = m_Instances.ExpandAndGetRef();
    inst.m_pObject = pObject;
    inst.m_Transform = transform;
  }

  /// \brief Checks whether a box would be visible, or is fully occluded by the existing scene geometry.
  ///
  /// Note: This only works after EndScene().
  bool IsVisible(const plSimdBBox& aabb) const;

  /// \brief Wether any occluder was actually added and also rasterized. If not, no need to do any visibility checks.
  bool HasRasterizedAnyOccluders() const
  {
    return m_bAnyOccludersRasterized;
  }

private:
  void SortObjectsFrontToBack();
  void RasterizeObjects(plUInt32 uiMaxObjects);
  void UpdateViewProjectionMatrix();
  void ApplyModelViewProjectionMatrix(const plTransform& modelTransform);

  bool m_bAnyOccludersRasterized = false;
  const plCamera* m_pCamera = nullptr;
  plUInt32 m_uiResolutionX = 0;
  plUInt32 m_uiResolutionY = 0;
  float m_fAspectRation = 1.0f;
  plUniquePtr<Rasterizer> m_pRasterizer;

  struct Instance
  {
    plTransform m_Transform;
    const plRasterizerObject* m_pObject;
  };

  plDeque<Instance> m_Instances;
  plMat4 m_mViewProjection;
};

class plRasterizerViewPool
{
public:
  plRasterizerView* GetRasterizerView(plUInt32 uiWidth, plUInt32 uiHeight, float fAspectRatio);
  void ReturnRasterizerView(plRasterizerView* pView);

private:
  struct PoolEntry
  {
    bool m_bInUse = false;
    plRasterizerView m_RasterizerView;
  };

  plMutex m_Mutex;
  plDeque<PoolEntry> m_Entries;
};
