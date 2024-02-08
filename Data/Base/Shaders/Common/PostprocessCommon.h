#pragma once

#include "ConstantBufferMacros.h"

#define POSTPROCESS_BLOCKSIZE 8

struct PL_SHADER_STRUCT plPostprocessTileStatistics
{
  UINT1(Dispatch_Earlyexit_ThreadGroupCountX);
  UINT1(Dispatch_Earlyexit_ThreadGroupCountY);
  UINT1(Dispatch_Earlyexit_ThreadGroupCountZ);

  UINT1(Dispatch_Cheap_ThreadGroupCountX);
  UINT1(Dispatch_Cheap_ThreadGroupCountY);
  UINT1(Dispatch_Cheap_ThreadGroupCountZ);

  UINT1(Dispatch_Expensive_ThreadGroupCountX);
  UINT1(Dispatch_Expensive_ThreadGroupCountY);
  UINT1(Dispatch_Expensive_ThreadGroupCountZ);
};

#if PL_DISABLED(PLATFORM_SHADER)
PL_DEFINE_AS_POD_TYPE(plPostprocessTileStatistics);
#endif

CONSTANT_BUFFER(plPostProcessingConstants, 3)
{
  FLOAT4(resolution);
  FLOAT4(params0);
  FLOAT4(params1);
};

