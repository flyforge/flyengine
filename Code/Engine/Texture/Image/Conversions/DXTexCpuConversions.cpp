#include <Texture/TexturePCH.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)

#  include <Texture/DirectXTex/BC.h>
#  include <Texture/Image/ImageConversion.h>

#  include <Foundation/Threading/TaskSystem.h>


plImageConversionEntry g_DXTexCpuConversions[] = {
  plImageConversionEntry(plImageFormat::R32G32B32A32_FLOAT, plImageFormat::BC6H_UF16, plImageConversionFlags::Default),

  plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM, plImageFormat::BC1_UNORM, plImageConversionFlags::Default),
  plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM, plImageFormat::BC7_UNORM, plImageConversionFlags::Default),

  plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM_SRGB, plImageFormat::BC1_UNORM_SRGB, plImageConversionFlags::Default),
  plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM_SRGB, plImageFormat::BC7_UNORM_SRGB, plImageConversionFlags::Default),
};

class plImageConversion_CompressDxTexCpu : public plImageConversionStepCompressBlocks
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    return g_DXTexCpuConversions;
  }

  virtual plResult CompressBlocks(plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 numBlocksX, plUInt32 numBlocksY,
    plImageFormat::Enum sourceFormat, plImageFormat::Enum targetFormat) const override
  {
    if (targetFormat == plImageFormat::BC7_UNORM || targetFormat == plImageFormat::BC7_UNORM_SRGB)
    {
      const plUInt32 srcStride = numBlocksX * 4 * 4;
      const plUInt32 targetStride = numBlocksX * 16;

      plTaskSystem::ParallelForIndexed(0, numBlocksY, [srcStride, targetStride, source, target, numBlocksX](plUInt32 startIndex, plUInt32 endIndex) {
        const plUInt8* srcIt = source.GetPtr() + srcStride * startIndex * 4;
        plUInt8* targetIt = target.GetPtr() + targetStride * startIndex;
        for (plUInt32 blockY = startIndex; blockY < endIndex; ++blockY)
        {
          for (plUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
          {
            DirectX::XMVECTOR temp[16];
            for (plUInt32 y = 0; y < 4; y++)
            {
              for (plUInt32 x = 0; x < 4; x++)
              {
                const plUInt8* pixel = srcIt + y * srcStride + x * 4;
                temp[y * 4 + x] = DirectX::XMVectorSet(pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, pixel[3] / 255.0f);
              }
            }
            DirectX::D3DXEncodeBC7(targetIt, temp, 0);

            srcIt += 4 * 4;
            targetIt += 16;
          }
          srcIt += 3 * srcStride;
        }
      });

      return PLASMA_SUCCESS;
    }
    else if (targetFormat == plImageFormat::BC1_UNORM || targetFormat == plImageFormat::BC1_UNORM_SRGB)
    {
      const plUInt32 srcStride = numBlocksX * 4 * 4;
      const plUInt32 targetStride = numBlocksX * 8;

      plTaskSystem::ParallelForIndexed(0, numBlocksY, [srcStride, targetStride, source, target, numBlocksX](plUInt32 startIndex, plUInt32 endIndex) {
        const plUInt8* srcIt = source.GetPtr() + srcStride * startIndex * 4;
        plUInt8* targetIt = target.GetPtr() + targetStride * startIndex;
        for (plUInt32 blockY = startIndex; blockY < endIndex; ++blockY)
        {
          for (plUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
          {
            DirectX::XMVECTOR temp[16];
            for (plUInt32 y = 0; y < 4; y++)
            {
              for (plUInt32 x = 0; x < 4; x++)
              {
                const plUInt8* pixel = srcIt + y * srcStride + x * 4;
                temp[y * 4 + x] = DirectX::XMVectorSet(pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, pixel[3] / 255.0f);
              }
            }
            DirectX::D3DXEncodeBC1(targetIt, temp, 1.0f, 0);

            srcIt += 4 * 4;
            targetIt += 8;
          }
          srcIt += 3 * srcStride;
        }
      });

      return PLASMA_SUCCESS;
    }
    else if (targetFormat == plImageFormat::BC6H_UF16)
    {
      const plUInt32 srcStride = numBlocksX * 4 * 4 * sizeof(float);
      const plUInt32 targetStride = numBlocksX * 16;

      plTaskSystem::ParallelForIndexed(0, numBlocksY, [srcStride, targetStride, source, target, numBlocksX](plUInt32 startIndex, plUInt32 endIndex) {
        const plUInt8* srcIt = source.GetPtr() + srcStride * startIndex * 4;
        plUInt8* targetIt = target.GetPtr() + targetStride * startIndex;
        for (plUInt32 blockY = startIndex; blockY < endIndex; ++blockY)
        {
          for (plUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
          {
            DirectX::XMVECTOR temp[16];
            for (plUInt32 y = 0; y < 4; y++)
            {
              for (plUInt32 x = 0; x < 4; x++)
              {
                const float* pixel = reinterpret_cast<const float*>(srcIt + y * srcStride + x * 4 * sizeof(float));
                temp[y * 4 + x] = DirectX::XMVectorSet(pixel[0], pixel[1], pixel[2], pixel[3]);
              }
            }
            DirectX::D3DXEncodeBC6HU(targetIt, temp, 0);

            srcIt += 4 * 4 * sizeof(float);
            targetIt += 16;
          }
          srcIt += 3 * srcStride;
        }
      });

      return PLASMA_SUCCESS;
    }

    return PLASMA_FAILURE;
  }
};

static plImageConversion_CompressDxTexCpu s_conversion_compressDxTexCpu;

#endif

PLASMA_STATICLINK_FILE(Texture, Texture_Image_Conversions_DXTexCpuConversions);