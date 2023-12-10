

inline plGALShaderCreationDescription::plGALShaderCreationDescription()
  : plHashableStruct()
{
}

inline plGALShaderCreationDescription::~plGALShaderCreationDescription()
{
  for (plUInt32 i = 0; i < plGALShaderStage::ENUM_COUNT; ++i)
  {
    plGALShaderByteCode* pByteCode = m_ByteCodes[i];
    m_ByteCodes[i] = nullptr;

    if (pByteCode != nullptr && pByteCode->GetRefCount() == 0)
    {
      PLASMA_DEFAULT_DELETE(pByteCode);
    }
  }
}

inline bool plGALShaderCreationDescription::HasByteCodeForStage(plGALShaderStage::Enum stage) const
{
  return m_ByteCodes[stage] != nullptr && m_ByteCodes[stage]->IsValid();
}

inline void plGALTextureCreationDescription::SetAsRenderTarget(
  plUInt32 uiWidth, plUInt32 uiHeight, plGALResourceFormat::Enum format, plGALMSAASampleCount::Enum sampleCount /*= plGALMSAASampleCount::None*/)
{
  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;
  m_uiDepth = 1;
  m_uiMipLevelCount = 1;
  m_uiArraySize = 1;
  m_SampleCount = sampleCount;
  m_Format = format;
  m_Type = plGALTextureType::Texture2D;
  m_bAllowShaderResourceView = true;
  m_bAllowUAV = false;
  m_bCreateRenderTarget = true;
  m_bAllowDynamicMipGeneration = false;
  m_ResourceAccess.m_bReadBack = false;
  m_ResourceAccess.m_bImmutable = true;
  m_pExisitingNativeObject = nullptr;
}

PLASMA_FORCE_INLINE plGALVertexAttribute::plGALVertexAttribute(
  plGALVertexAttributeSemantic::Enum semantic, plGALResourceFormat::Enum format, plUInt16 uiOffset, plUInt8 uiVertexBufferSlot, bool bInstanceData)
  : m_eSemantic(semantic)
  , m_eFormat(format)
  , m_uiOffset(uiOffset)
  , m_uiVertexBufferSlot(uiVertexBufferSlot)
  , m_bInstanceData(bInstanceData)
{
}
