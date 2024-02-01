
inline plBitflags<plGALShaderResourceCategory> plGALShaderResourceCategory::MakeFromShaderDescriptorType(plGALShaderResourceType::Enum type)
{
  switch (type)
  {
    case plGALShaderResourceType::Sampler:
      return plGALShaderResourceCategory::Sampler;
    case plGALShaderResourceType::ConstantBuffer:
    case plGALShaderResourceType::PushConstants:
      return plGALShaderResourceCategory::ConstantBuffer;
    case plGALShaderResourceType::Texture:
    case plGALShaderResourceType::TexelBuffer:
    case plGALShaderResourceType::StructuredBuffer:
      return plGALShaderResourceCategory::SRV;
    case plGALShaderResourceType::TextureRW:
    case plGALShaderResourceType::TexelBufferRW:
    case plGALShaderResourceType::StructuredBufferRW:
      return plGALShaderResourceCategory::UAV;
    case plGALShaderResourceType::TextureAndSampler:
      return plGALShaderResourceCategory::SRV | plGALShaderResourceCategory::Sampler;
    default:
      PL_REPORT_FAILURE("Missing enum");
      return {};
  }
}

inline bool plGALShaderTextureType::IsArray(plGALShaderTextureType::Enum format)
{
  switch (format)
  {
    case plGALShaderTextureType::Texture1DArray:
    case plGALShaderTextureType::Texture2DArray:
    case plGALShaderTextureType::Texture2DMSArray:
    case plGALShaderTextureType::TextureCubeArray:
      return true;
    default:
      return false;
  }
}