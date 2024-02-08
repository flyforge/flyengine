#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Pipeline/ViewRenderMode.h>
#include <RendererFoundation/Device/SwapChain.h>

/// \brief Holds view data like the viewport, view and projection matrices
struct PL_RENDERERCORE_DLL plViewData
{
  plViewData()
  {
    m_ViewPortRect = plRectFloat(0.0f, 0.0f);
    m_ViewRenderMode = plViewRenderMode::None;

    for (int i = 0; i < 2; ++i)
    {
      m_ViewMatrix[i].SetIdentity();
      m_InverseViewMatrix[i].SetIdentity();
      m_ProjectionMatrix[i].SetIdentity();
      m_InverseProjectionMatrix[i].SetIdentity();
      m_ViewProjectionMatrix[i].SetIdentity();
      m_InverseViewProjectionMatrix[i].SetIdentity();

      m_LastViewMatrix[i].SetIdentity();
      m_LastInverseViewMatrix[i].SetIdentity();
      m_LastProjectionMatrix[i].SetIdentity();
      m_LastInverseProjectionMatrix[i].SetIdentity();
      m_LastViewProjectionMatrix[i].SetIdentity();
      m_LastInverseViewProjectionMatrix[i].SetIdentity();
    }
  }

  plGALRenderTargets m_renderTargets;
  plGALSwapChainHandle m_hSwapChain;
  plRectFloat m_ViewPortRect;
  plEnum<plViewRenderMode> m_ViewRenderMode;
  plEnum<plCameraUsageHint> m_CameraUsageHint;

  // Each matrix is there for both left and right camera lens.
  plMat4 m_ViewMatrix[2];
  plMat4 m_InverseViewMatrix[2];
  plMat4 m_ProjectionMatrix[2];
  plMat4 m_InverseProjectionMatrix[2];
  plMat4 m_ViewProjectionMatrix[2];
  plMat4 m_InverseViewProjectionMatrix[2];

  plMat4 m_LastViewMatrix[2];
  plMat4 m_LastInverseViewMatrix[2];
  plMat4 m_LastProjectionMatrix[2];
  plMat4 m_LastInverseProjectionMatrix[2];
  plMat4 m_LastViewProjectionMatrix[2];
  plMat4 m_LastInverseViewProjectionMatrix[2];

  /// \brief Returns the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fScreenPosX and fScreenPosY are expected to be in [0; 1] range (normalized pixel coordinates).
  /// If no ray can be computed, PL_FAILURE is returned.
  plResult ComputePickingRay(
    float fScreenPosX, float fScreenPosY, plVec3& out_vRayStartPos, plVec3& out_vRayDir, plCameraEye eye = plCameraEye::Left) const
  {
    plVec3 vScreenPos;
    vScreenPos.x = fScreenPosX;
    vScreenPos.y = 1.0f - fScreenPosY;
    vScreenPos.z = 0.0f;

    return plGraphicsUtils::ConvertScreenPosToWorldPos(
      m_InverseViewProjectionMatrix[static_cast<int>(eye)], 0, 0, 1, 1, vScreenPos, out_vRayStartPos, &out_vRayDir);
  }

  plResult ComputeScreenSpacePos(const plVec3& vPoint, plVec3& out_vScreenPos, plCameraEye eye = plCameraEye::Left) const
  {
    plUInt32 x = (plUInt32)m_ViewPortRect.x;
    plUInt32 y = (plUInt32)m_ViewPortRect.y;
    plUInt32 w = (plUInt32)m_ViewPortRect.width;
    plUInt32 h = (plUInt32)m_ViewPortRect.height;

    if (plGraphicsUtils::ConvertWorldPosToScreenPos(m_ViewProjectionMatrix[static_cast<int>(eye)], x, y, w, h, vPoint, out_vScreenPos).Succeeded())
    {
      out_vScreenPos.y = m_ViewPortRect.height - out_vScreenPos.y;

      return PL_SUCCESS;
    }

    return PL_FAILURE;
  }
};
