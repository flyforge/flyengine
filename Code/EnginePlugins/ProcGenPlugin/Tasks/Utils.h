#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>

class plVolumeCollection;

struct PL_PROCGENPLUGIN_DLL plProcGenExpressionFunctions
{
  static plExpressionFunction s_ApplyVolumesFunc;
  static plExpressionFunction s_GetInstanceSeedFunc;
};

namespace plProcGenInternal
{
  void ExtractVolumeCollections(const plWorld& world, const plBoundingBox& box, const Output& output, plDeque<plVolumeCollection>& ref_volumeCollections, plExpression::GlobalData& ref_globalData);

  void SetInstanceSeed(plUInt32 uiSeed, plExpression::GlobalData& ref_globalData);
} // namespace plProcGenInternal
