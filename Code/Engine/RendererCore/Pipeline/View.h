#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/TagSet.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelineNode.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class plFrustum;
class plWorld;
class plRenderPipeline;

/// \brief Encapsulates a view on the given world through the given camera
/// and rendered with the specified RenderPipeline into the given render target setup.
class PL_RENDERERCORE_DLL plView : public plRenderPipelineNode
{
  PL_ADD_DYNAMIC_REFLECTION(plView, plRenderPipelineNode);

private:
  /// \brief Use plRenderLoop::CreateView to create a view.
  plView();
  ~plView();

public:
  plViewHandle GetHandle() const;

  void SetName(plStringView sName);
  plStringView GetName() const;

  void SetWorld(plWorld* pWorld);
  plWorld* GetWorld();
  const plWorld* GetWorld() const;

  /// \brief Sets the swapchain that this view will be rendering into. Can be invalid in case the render target is an off-screen buffer in which case SetRenderTargets needs to be called.
  /// Setting the swap-chain is necessary in order to acquire and present the image to the window.
  /// SetSwapChain and SetRenderTargets are mutually exclusive. Calling this function will reset the render targets.
  void SetSwapChain(plGALSwapChainHandle hSwapChain);
  plGALSwapChainHandle GetSwapChain() const;

  /// \brief Sets the off-screen render targets. Use SetSwapChain if rendering to a window.
  /// SetSwapChain and SetRenderTargets are mutually exclusive. Calling this function will reset the swap chain.
  void SetRenderTargets(const plGALRenderTargets& renderTargets);
  const plGALRenderTargets& GetRenderTargets() const;

  /// \brief Returns the render targets that were either set via the swapchain or via the manually set render targets.
  const plGALRenderTargets& GetActiveRenderTargets() const;

  void SetRenderPipelineResource(plRenderPipelineResourceHandle hPipeline);
  plRenderPipelineResourceHandle GetRenderPipelineResource() const;

  void SetCamera(plCamera* pCamera);
  plCamera* GetCamera();
  const plCamera* GetCamera() const;

  void SetCullingCamera(const plCamera* pCamera);
  const plCamera* GetCullingCamera() const;

  void SetLodCamera(const plCamera* pCamera);
  const plCamera* GetLodCamera() const;

  /// \brief Returns the camera usage hint for the view.
  plEnum<plCameraUsageHint> GetCameraUsageHint() const;
  /// \brief Sets the camera usage hint for the view. If not 'None', the camera component of the same usage will be auto-connected
  ///   to this view.
  void SetCameraUsageHint(plEnum<plCameraUsageHint> val);

  void SetViewRenderMode(plEnum<plViewRenderMode> value);
  plEnum<plViewRenderMode> GetViewRenderMode() const;

  void SetViewport(const plRectFloat& viewport);
  const plRectFloat& GetViewport() const;

  /// \brief Forces the render pipeline to be rebuilt.
  void ForceUpdate();

  const plViewData& GetData() const;

  bool IsValid() const;

  /// \brief Extracts all relevant data from the world to render the view.
  void ExtractData();

  /// \brief Returns a task implementation that calls ExtractData on this view.
  const plSharedPtr<plTask>& GetExtractTask();


  /// \brief Returns the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fScreenPosX and fScreenPosY are expected to be in [0; 1] range (normalized pixel coordinates).
  /// If no ray can be computed, PL_FAILURE is returned.
  plResult ComputePickingRay(float fScreenPosX, float fScreenPosY, plVec3& out_vRayStartPos, plVec3& out_vRayDir) const;

  plResult ComputeScreenSpacePos(const plVec3& vPoint, plVec3& out_vScreenPos) const;

  /// \brief Returns the current projection matrix.
  const plMat4& GetProjectionMatrix(plCameraEye eye) const;

  /// \brief Returns the current inverse projection matrix.
  const plMat4& GetInverseProjectionMatrix(plCameraEye eye) const;

  /// \brief Returns the current view matrix (camera orientation).
  const plMat4& GetViewMatrix(plCameraEye eye) const;

  /// \brief Returns the current inverse view matrix (inverse camera orientation).
  const plMat4& GetInverseViewMatrix(plCameraEye eye) const;

  /// \brief Returns the current view-projection matrix.
  const plMat4& GetViewProjectionMatrix(plCameraEye eye) const;

  /// \brief Returns the current inverse view-projection matrix.
  const plMat4& GetInverseViewProjectionMatrix(plCameraEye eye) const;

  /// \brief Returns the frustum that should be used for determine visible objects for this view.
  void ComputeCullingFrustum(plFrustum& out_frustum) const;

  void SetShaderPermutationVariable(const char* szName, const char* szValue);

  void SetRenderPassProperty(const char* szPassName, const char* szPropertyName, const plVariant& value);
  void SetExtractorProperty(const char* szPassName, const char* szPropertyName, const plVariant& value);

  void ResetRenderPassProperties();
  void ResetExtractorProperties();

  void SetRenderPassReadBackProperty(const char* szPassName, const char* szPropertyName, const plVariant& value);
  plVariant GetRenderPassReadBackProperty(const char* szPassName, const char* szPropertyName);
  bool IsRenderPassReadBackPropertyExisting(const char* szPassName, const char* szPropertyName) const;

  /// \brief Pushes the view and camera data into the extracted data of the pipeline.
  ///
  /// Use plRenderWorld::GetDataIndexForExtraction() to update the data from the extraction thread. Can't be used if this view is currently extracted.
  /// Use plRenderWorld::GetDataIndexForRendering() to update the data from the render thread.
  void UpdateViewData(plUInt32 uiDataIndex);

  plTagSet m_IncludeTags;
  plTagSet m_ExcludeTags;

private:
  friend class plRenderWorld;
  friend class plMemoryUtils;

  plViewId m_InternalId;
  plHashedString m_sName;

  plSharedPtr<plTask> m_pExtractTask;

  plWorld* m_pWorld = nullptr;

  plRenderPipelineResourceHandle m_hRenderPipeline;
  plUInt32 m_uiRenderPipelineResourceDescriptionCounter = 0;
  plSharedPtr<plRenderPipeline> m_pRenderPipeline;
  plCamera* m_pCamera = nullptr;
  const plCamera* m_pCullingCamera = nullptr;
  const plCamera* m_pLodCamera = nullptr;


private:
  plRenderPipelineNodeInputPin m_PinRenderTarget0;
  plRenderPipelineNodeInputPin m_PinRenderTarget1;
  plRenderPipelineNodeInputPin m_PinRenderTarget2;
  plRenderPipelineNodeInputPin m_PinRenderTarget3;
  plRenderPipelineNodeInputPin m_PinDepthStencil;

private:
  void UpdateCachedMatrices() const;

  /// \brief Rebuilds pipeline if necessary and pushes double-buffered settings into the pipeline.
  void EnsureUpToDate();

  mutable plUInt32 m_uiLastCameraSettingsModification = 0;
  mutable plUInt32 m_uiLastCameraOrientationModification = 0;
  mutable float m_fLastViewportAspectRatio = 1.0f;

  mutable plViewData m_Data;

  plInternal::RenderDataCache* m_pRenderDataCache = nullptr;

  plDynamicArray<plPermutationVar> m_PermutationVars;
  bool m_bPermutationVarsDirty = false;

  void ApplyPermutationVars();

  struct PropertyValue
  {
    plString m_sObjectName;
    plString m_sPropertyName;
    plVariant m_DefaultValue;
    plVariant m_CurrentValue;
    bool m_bIsValid;
    bool m_bIsDirty;
  };

  void SetProperty(plMap<plString, PropertyValue>& map, const char* szPassName, const char* szPropertyName, const plVariant& value);
  void SetReadBackProperty(plMap<plString, PropertyValue>& map, const char* szPassName, const char* szPropertyName, const plVariant& value);

  void ReadBackPassProperties();

  void ResetAllPropertyStates(plMap<plString, PropertyValue>& map);

  void ApplyRenderPassProperties();
  void ApplyExtractorProperties();

  void ApplyProperty(plReflectedClass* pObject, PropertyValue& data, const char* szTypeName);

  plMap<plString, PropertyValue> m_PassProperties;
  plMap<plString, PropertyValue> m_PassReadBackProperties;
  plMap<plString, PropertyValue> m_ExtractorProperties;
};

#include <RendererCore/Pipeline/Implementation/View_inl.h>
