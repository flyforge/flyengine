#include <Shaders/Common/GlobalConstants.h>

CONSTANT_BUFFER(plMaterialConstants, 1)
{
  COLOR4F(BaseColor);
    // --- 16 Byte Split
  COLOR4F(EmissiveColor);
    // --- 16 Byte Split
  FLOAT1(MetallicValue);
  FLOAT1(ReflectanceValue);
  FLOAT1(RoughnessValue);
  FLOAT1(MaskThreshold);
    // --- 16 Byte Split
  BOOL1(UseBaseTexture);
  BOOL1(UseNormalTexture);
  BOOL1(UseRoughnessTexture);
  BOOL1(UseMetallicTexture);
    // --- 16 Byte Split
  BOOL1(UseEmissiveTexture);
  BOOL1(UseOcclusionTexture);
  FLOAT2(Tiling);
  // --- 16 Byte Split
  BOOL1(UseOrmTexture);
  BOOL1(InvertOcclusion);
  BOOL1(UseRMA);
};
