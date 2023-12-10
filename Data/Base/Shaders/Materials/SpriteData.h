#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>

struct PLASMA_SHADER_STRUCT plPerSpriteData
{
  FLOAT3(WorldSpacePosition);
  FLOAT1(Size);
  FLOAT1(MaxScreenSize);
  FLOAT1(AspectRatio);
  UINT1(ColorRG);
  UINT1(ColorBA);
  UINT1(TexCoordScale);
  UINT1(TexCoordOffset);
  UINT1(GameObjectID);
  UINT1(Reserved);
};
