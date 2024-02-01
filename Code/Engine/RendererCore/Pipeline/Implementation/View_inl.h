
PL_ALWAYS_INLINE plViewHandle plView::GetHandle() const
{
  return plViewHandle(m_InternalId);
}

PL_ALWAYS_INLINE plStringView plView::GetName() const
{
  return m_sName.GetView();
}

PL_ALWAYS_INLINE plWorld* plView::GetWorld()
{
  return m_pWorld;
}

PL_ALWAYS_INLINE const plWorld* plView::GetWorld() const
{
  return m_pWorld;
}

PL_ALWAYS_INLINE plGALSwapChainHandle plView::GetSwapChain() const
{
  return m_Data.m_hSwapChain;
}

PL_ALWAYS_INLINE const plGALRenderTargets& plView::GetRenderTargets() const
{
  return m_Data.m_renderTargets;
}

PL_ALWAYS_INLINE void plView::SetCamera(plCamera* pCamera)
{
  m_pCamera = pCamera;
}

PL_ALWAYS_INLINE plCamera* plView::GetCamera()
{
  return m_pCamera;
}

PL_ALWAYS_INLINE const plCamera* plView::GetCamera() const
{
  return m_pCamera;
}

PL_ALWAYS_INLINE void plView::SetCullingCamera(const plCamera* pCamera)
{
  m_pCullingCamera = pCamera;
}

PL_ALWAYS_INLINE const plCamera* plView::GetCullingCamera() const
{
  return m_pCullingCamera != nullptr ? m_pCullingCamera : m_pCamera;
}

PL_ALWAYS_INLINE void plView::SetLodCamera(const plCamera* pCamera)
{
  m_pLodCamera = pCamera;
}

PL_ALWAYS_INLINE const plCamera* plView::GetLodCamera() const
{
  return m_pLodCamera != nullptr ? m_pLodCamera : m_pCamera;
}

PL_ALWAYS_INLINE plEnum<plCameraUsageHint> plView::GetCameraUsageHint() const
{
  return m_Data.m_CameraUsageHint;
}

PL_ALWAYS_INLINE plEnum<plViewRenderMode> plView::GetViewRenderMode() const
{
  return m_Data.m_ViewRenderMode;
}

PL_ALWAYS_INLINE const plRectFloat& plView::GetViewport() const
{
  return m_Data.m_ViewPortRect;
}

PL_ALWAYS_INLINE const plViewData& plView::GetData() const
{
  UpdateCachedMatrices();
  return m_Data;
}

PL_FORCE_INLINE bool plView::IsValid() const
{
  return m_pWorld != nullptr && m_pRenderPipeline != nullptr && m_pCamera != nullptr && m_Data.m_ViewPortRect.HasNonZeroArea();
}

PL_ALWAYS_INLINE const plSharedPtr<plTask>& plView::GetExtractTask()
{
  return m_pExtractTask;
}

PL_FORCE_INLINE plResult plView::ComputePickingRay(float fScreenPosX, float fScreenPosY, plVec3& out_vRayStartPos, plVec3& out_vRayDir) const
{
  UpdateCachedMatrices();
  return m_Data.ComputePickingRay(fScreenPosX, fScreenPosY, out_vRayStartPos, out_vRayDir);
}

PL_FORCE_INLINE plResult plView::ComputeScreenSpacePos(const plVec3& vPoint, plVec3& out_vScreenPos) const
{
  UpdateCachedMatrices();
  return m_Data.ComputeScreenSpacePos(vPoint, out_vScreenPos);
}

PL_ALWAYS_INLINE const plMat4& plView::GetProjectionMatrix(plCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ProjectionMatrix[static_cast<int>(eye)];
}

PL_ALWAYS_INLINE const plMat4& plView::GetInverseProjectionMatrix(plCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseProjectionMatrix[static_cast<int>(eye)];
}

PL_ALWAYS_INLINE const plMat4& plView::GetViewMatrix(plCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ViewMatrix[static_cast<int>(eye)];
}

PL_ALWAYS_INLINE const plMat4& plView::GetInverseViewMatrix(plCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseViewMatrix[static_cast<int>(eye)];
}

PL_ALWAYS_INLINE const plMat4& plView::GetViewProjectionMatrix(plCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ViewProjectionMatrix[static_cast<int>(eye)];
}

PL_ALWAYS_INLINE const plMat4& plView::GetInverseViewProjectionMatrix(plCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseViewProjectionMatrix[static_cast<int>(eye)];
}
