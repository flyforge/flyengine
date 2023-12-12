#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class PLASMA_RENDERERCORE_DLL plRenderSortingFunctions
{
public:
  static plUInt64 ByRenderDataThenFrontToBack(const plRenderData* pRenderData, plUInt32 uiRenderDataSortingKey, const plCamera& camera);
  static plUInt64 BackToFrontThenByRenderData(const plRenderData* pRenderData, plUInt32 uiRenderDataSortingKey, const plCamera& camera);
};
