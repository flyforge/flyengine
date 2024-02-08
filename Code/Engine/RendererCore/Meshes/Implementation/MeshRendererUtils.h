#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

namespace plInternal
{
  PL_FORCE_INLINE void FillPerInstanceData(plPerInstanceData& ref_perInstanceData, const plMeshRenderData* pRenderData)
  {
    plMat4 objectToWorld = pRenderData->m_GlobalTransform.GetAsMat4();

    ref_perInstanceData.ObjectToWorld = objectToWorld;

    #if PL_ENABLED(PL_GAMEOBJECT_VELOCITY)
      plMat4 lastObjectToWorld = pRenderData->m_LastGlobalTransform.GetAsMat4();
      ref_perInstanceData.LastObjectToWorld = lastObjectToWorld;
    #endif

    if (pRenderData->m_uiUniformScale)
    {
      ref_perInstanceData.ObjectToWorldNormal = objectToWorld;

      #if PL_ENABLED(PL_GAMEOBJECT_VELOCITY)
        ref_perInstanceData.LastObjectToWorldNormal = lastObjectToWorld;
      #endif
    }
    else
    {
      plMat3 mInverse = objectToWorld.GetRotationalPart();
      mInverse.Invert(0.0f).IgnoreResult();
      // we explicitly ignore the return value here (success / failure)
      // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying

      plShaderTransform shaderT;
      shaderT = mInverse.GetTranspose();
      ref_perInstanceData.ObjectToWorldNormal = shaderT;

      #if PL_ENABLED(PL_GAMEOBJECT_VELOCITY)
        mInverse = lastObjectToWorld.GetRotationalPart();
        mInverse.Invert(0.0f).IgnoreResult();
        // we explicitly ignore the return value here (success / failure)
        // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying

        shaderT = mInverse.GetTranspose();
        ref_perInstanceData.LastObjectToWorldNormal = shaderT;
      #endif
    }

    ref_perInstanceData.BoundingSphereRadius = pRenderData->m_GlobalBounds.m_fSphereRadius;
    ref_perInstanceData.GameObjectID = pRenderData->m_uiUniqueID;
    ref_perInstanceData.VertexColorAccessData = 0;
    ref_perInstanceData.Color = pRenderData->m_Color;
  }
} // namespace plInternal
