#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>

class plVolumeCollection;

struct PLASMA_PROCGENPLUGIN_DLL plProcGenExpressionFunctions
{
  static plExpressionFunction s_ApplyVolumesFunc;
  static plExpressionFunction s_GetInstanceSeedFunc;
};

namespace plProcGenInternal
{
  void ExtractVolumeCollections(const plWorld& world, const plBoundingBox& box, const Output& output, plDeque<plVolumeCollection>& volumeCollections, plExpression::GlobalData& globalData);

  void SetInstanceSeed(plUInt32 uiSeed, plExpression::GlobalData& globalData);
} // namespace plProcGenInternal