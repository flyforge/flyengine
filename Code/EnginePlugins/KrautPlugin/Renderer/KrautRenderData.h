#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <KrautPlugin/KrautDeclarations.h>
#include <RendererCore/Pipeline/RenderData.h>

using plMeshResourceHandle = plTypedResourceHandle<class plMeshResource>;

class PLASMA_KRAUTPLUGIN_DLL plKrautRenderData : public plRenderData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plKrautRenderData, plRenderData);

public:
  plMeshResourceHandle m_hMesh;
  plUInt32 m_uiUniqueID = 0;
  float m_fLodDistanceMinSQR;
  float m_fLodDistanceMaxSQR;
  plVec3 m_vLeafCenter;

  plUInt8 m_uiSubMeshIndex = 0;
  plUInt8 m_uiThisLodIndex = 0;
  bool m_bCastShadows = false;
  plVec3 m_vWindTrunk = plVec3::MakeZero();
  plVec3 m_vWindBranches = plVec3::MakeZero();
};
