#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class PL_RENDERERCORE_DLL plRenderSortingFunctions
{
public:
  static plUInt64 ByRenderDataThenFrontToBack(const plRenderData* pRenderData, const plCamera& camera);
  static plUInt64 BackToFrontThenByRenderData(const plRenderData* pRenderData, const plCamera& camera);
};
