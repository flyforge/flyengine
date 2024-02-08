
/// \brief Used to guard plGALDevice functions from multi-threaded access and to verify that executing them on non-main-threads is allowed
#define PL_GALDEVICE_LOCK_AND_CHECK()                                                                                                                \
  PL_LOCK(m_Mutex);                                                                                                                                  \
  VerifyMultithreadedAccess()

PL_ALWAYS_INLINE const plGALDeviceCreationDescription* plGALDevice::GetDescription() const
{
  return &m_Description;
}

PL_ALWAYS_INLINE plResult plGALDevice::GetTimestampResult(plGALTimestampHandle hTimestamp, plTime& ref_result)
{
  return GetTimestampResultPlatform(hTimestamp, ref_result);
}

PL_ALWAYS_INLINE plGALTimestampHandle plGALDevice::GetTimestamp()
{
  return GetTimestampPlatform();
}

template <typename IdTableType, typename ReturnType>
PL_ALWAYS_INLINE ReturnType* plGALDevice::Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const
{
  PL_GALDEVICE_LOCK_AND_CHECK();

  ReturnType* pObject = nullptr;
  IdTable.TryGetValue(hHandle, pObject);
  return pObject;
}

inline const plGALSwapChain* plGALDevice::GetSwapChain(plGALSwapChainHandle hSwapChain) const
{
  return Get<SwapChainTable, plGALSwapChain>(hSwapChain, m_SwapChains);
}

inline const plGALShader* plGALDevice::GetShader(plGALShaderHandle hShader) const
{
  return Get<ShaderTable, plGALShader>(hShader, m_Shaders);
}

inline const plGALTexture* plGALDevice::GetTexture(plGALTextureHandle hTexture) const
{
  return Get<TextureTable, plGALTexture>(hTexture, m_Textures);
}

inline const plGALBuffer* plGALDevice::GetBuffer(plGALBufferHandle hBuffer) const
{
  return Get<BufferTable, plGALBuffer>(hBuffer, m_Buffers);
}

inline const plGALDepthStencilState* plGALDevice::GetDepthStencilState(plGALDepthStencilStateHandle hDepthStencilState) const
{
  return Get<DepthStencilStateTable, plGALDepthStencilState>(hDepthStencilState, m_DepthStencilStates);
}

inline const plGALBlendState* plGALDevice::GetBlendState(plGALBlendStateHandle hBlendState) const
{
  return Get<BlendStateTable, plGALBlendState>(hBlendState, m_BlendStates);
}

inline const plGALRasterizerState* plGALDevice::GetRasterizerState(plGALRasterizerStateHandle hRasterizerState) const
{
  return Get<RasterizerStateTable, plGALRasterizerState>(hRasterizerState, m_RasterizerStates);
}

inline const plGALVertexDeclaration* plGALDevice::GetVertexDeclaration(plGALVertexDeclarationHandle hVertexDeclaration) const
{
  return Get<VertexDeclarationTable, plGALVertexDeclaration>(hVertexDeclaration, m_VertexDeclarations);
}

inline const plGALSamplerState* plGALDevice::GetSamplerState(plGALSamplerStateHandle hSamplerState) const
{
  return Get<SamplerStateTable, plGALSamplerState>(hSamplerState, m_SamplerStates);
}

inline const plGALResourceView* plGALDevice::GetResourceView(plGALResourceViewHandle hResourceView) const
{
  return Get<ResourceViewTable, plGALResourceView>(hResourceView, m_ResourceViews);
}

inline const plGALRenderTargetView* plGALDevice::GetRenderTargetView(plGALRenderTargetViewHandle hRenderTargetView) const
{
  return Get<RenderTargetViewTable, plGALRenderTargetView>(hRenderTargetView, m_RenderTargetViews);
}

inline const plGALUnorderedAccessView* plGALDevice::GetUnorderedAccessView(plGALUnorderedAccessViewHandle hUnorderedAccessView) const
{
  return Get<UnorderedAccessViewTable, plGALUnorderedAccessView>(hUnorderedAccessView, m_UnorderedAccessViews);
}

inline const plGALQuery* plGALDevice::GetQuery(plGALQueryHandle hQuery) const
{
  return Get<QueryTable, plGALQuery>(hQuery, m_Queries);
}

// static
PL_ALWAYS_INLINE void plGALDevice::SetDefaultDevice(plGALDevice* pDefaultDevice)
{
  s_pDefaultDevice = pDefaultDevice;
}

// static
PL_ALWAYS_INLINE plGALDevice* plGALDevice::GetDefaultDevice()
{
  PL_ASSERT_DEBUG(s_pDefaultDevice != nullptr, "Default device not set.");
  return s_pDefaultDevice;
}

// static
PL_ALWAYS_INLINE bool plGALDevice::HasDefaultDevice()
{
  return s_pDefaultDevice != nullptr;
}

PL_ALWAYS_INLINE bool plGALDevice::GetRayTracingSupported()
{
  return false;
}

template <typename HandleType>
PL_FORCE_INLINE void plGALDevice::AddDeadObject(plUInt32 uiType, HandleType handle)
{
  auto& deadObject = m_DeadObjects.ExpandAndGetRef();
  deadObject.m_uiType = uiType;
  deadObject.m_uiHandle = handle.GetInternalID().m_Data;
}

template <typename HandleType>
void plGALDevice::ReviveDeadObject(plUInt32 uiType, HandleType handle)
{
  plUInt32 uiHandle = handle.GetInternalID().m_Data;

  for (plUInt32 i = 0; i < m_DeadObjects.GetCount(); ++i)
  {
    const auto& deadObject = m_DeadObjects[i];

    if (deadObject.m_uiType == uiType && deadObject.m_uiHandle == uiHandle)
    {
      m_DeadObjects.RemoveAtAndCopy(i);
      return;
    }
  }
}

PL_ALWAYS_INLINE void plGALDevice::VerifyMultithreadedAccess() const
{
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  PL_ASSERT_DEV(m_Capabilities.m_bMultithreadedResourceCreation || plThreadUtils::IsMainThread(),
    "This device does not support multi-threaded resource creation, therefore this function can only be executed on the main thread.");
#endif
}
