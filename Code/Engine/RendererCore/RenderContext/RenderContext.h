#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Strings/String.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/GlobalConstants.h>

struct plRenderWorldRenderEvent;

//////////////////////////////////////////////////////////////////////////
// plRenderContext
//////////////////////////////////////////////////////////////////////////

class PL_RENDERERCORE_DLL plRenderContext
{
private:
  plRenderContext();
  ~plRenderContext();
  friend class plMemoryUtils;

  static plRenderContext* s_pDefaultInstance;
  static plHybridArray<plRenderContext*, 4> s_Instances;

public:
  static plRenderContext* GetDefaultInstance();
  static plRenderContext* CreateInstance();
  static void DestroyInstance(plRenderContext* pRenderer);

public:
  struct Statistics
  {
    Statistics();
    void Reset();

    plUInt32 m_uiFailedDrawcalls;
  };

  Statistics GetAndResetStatistics();

  plGALRenderCommandEncoder* BeginRendering(plGALPass* pGALPass, const plGALRenderingSetup& renderingSetup, const plRectFloat& viewport, const char* szName = "", bool bStereoRendering = false);
  void EndRendering();

  plGALComputeCommandEncoder* BeginCompute(plGALPass* pGALPass, const char* szName = "");
  void EndCompute();

  // Helper class to automatically end rendering or compute on scope exit
  template <typename T>
  class CommandEncoderScope
  {
    PL_DISALLOW_COPY_AND_ASSIGN(CommandEncoderScope);

  public:
    PL_ALWAYS_INLINE ~CommandEncoderScope()
    {
      m_RenderContext.EndCommandEncoder(m_pGALCommandEncoder);

      if (m_pGALPass != nullptr)
      {
        plGALDevice::GetDefaultDevice()->EndPass(m_pGALPass);
      }
    }

    PL_ALWAYS_INLINE T* operator->() { return m_pGALCommandEncoder; }
    PL_ALWAYS_INLINE operator const T*() { return m_pGALCommandEncoder; }

  private:
    friend class plRenderContext;

    PL_ALWAYS_INLINE CommandEncoderScope(plRenderContext& renderContext, plGALPass* pGALPass, T* pGALCommandEncoder)
      : m_RenderContext(renderContext)
      , m_pGALPass(pGALPass)
      , m_pGALCommandEncoder(pGALCommandEncoder)
    {
    }

    plRenderContext& m_RenderContext;
    plGALPass* m_pGALPass;
    T* m_pGALCommandEncoder;
  };

  using RenderingScope = CommandEncoderScope<plGALRenderCommandEncoder>;
  PL_ALWAYS_INLINE static RenderingScope BeginRenderingScope(plGALPass* pGALPass, const plRenderViewContext& viewContext, const plGALRenderingSetup& renderingSetup, const char* szName = "", bool bStereoRendering = false)
  {
    return RenderingScope(*viewContext.m_pRenderContext, nullptr, viewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, viewContext.m_pViewData->m_ViewPortRect, szName, bStereoRendering));
  }

  PL_ALWAYS_INLINE static RenderingScope BeginPassAndRenderingScope(const plRenderViewContext& viewContext, const plGALRenderingSetup& renderingSetup, const char* szName, bool bStereoRendering = false)
  {
    plGALPass* pGALPass = plGALDevice::GetDefaultDevice()->BeginPass(szName);

    return RenderingScope(*viewContext.m_pRenderContext, pGALPass, viewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, viewContext.m_pViewData->m_ViewPortRect, "", bStereoRendering));
  }

  using ComputeScope = CommandEncoderScope<plGALComputeCommandEncoder>;
  PL_ALWAYS_INLINE static ComputeScope BeginComputeScope(plGALPass* pGALPass, const plRenderViewContext& viewContext, const char* szName = "")
  {
    return ComputeScope(*viewContext.m_pRenderContext, nullptr, viewContext.m_pRenderContext->BeginCompute(pGALPass, szName));
  }

  PL_ALWAYS_INLINE static ComputeScope BeginPassAndComputeScope(const plRenderViewContext& viewContext, const char* szName)
  {
    plGALPass* pGALPass = plGALDevice::GetDefaultDevice()->BeginPass(szName);

    return ComputeScope(*viewContext.m_pRenderContext, pGALPass, viewContext.m_pRenderContext->BeginCompute(pGALPass));
  }

  PL_ALWAYS_INLINE plGALCommandEncoder* GetCommandEncoder()
  {
    PL_ASSERT_DEBUG(m_pGALCommandEncoder != nullptr, "BeginRendering/Compute has not been called");
    return m_pGALCommandEncoder;
  }

  PL_ALWAYS_INLINE plGALRenderCommandEncoder* GetRenderCommandEncoder()
  {
    PL_ASSERT_DEBUG(m_pGALCommandEncoder != nullptr && !m_bCompute, "BeginRendering has not been called");
    return static_cast<plGALRenderCommandEncoder*>(m_pGALCommandEncoder);
  }

  PL_ALWAYS_INLINE plGALComputeCommandEncoder* GetComputeCommandEncoder()
  {
    PL_ASSERT_DEBUG(m_pGALCommandEncoder != nullptr && m_bCompute, "BeginCompute has not been called");
    return static_cast<plGALComputeCommandEncoder*>(m_pGALCommandEncoder);
  }


  // Member Functions
  void SetShaderPermutationVariable(const char* szName, const plTempHashedString& sValue);
  void SetShaderPermutationVariable(const plHashedString& sName, const plHashedString& sValue);

  void BindMaterial(const plMaterialResourceHandle& hMaterial);

  void BindTexture2D(const plTempHashedString& sSlotName, const plTexture2DResourceHandle& hTexture, plResourceAcquireMode acquireMode = plResourceAcquireMode::AllowLoadingFallback);
  void BindTexture3D(const plTempHashedString& sSlotName, const plTexture3DResourceHandle& hTexture, plResourceAcquireMode acquireMode = plResourceAcquireMode::AllowLoadingFallback);
  void BindTextureCube(const plTempHashedString& sSlotName, const plTextureCubeResourceHandle& hTexture, plResourceAcquireMode acquireMode = plResourceAcquireMode::AllowLoadingFallback);

  void BindTexture2D(const plTempHashedString& sSlotName, plGALResourceViewHandle hResourceView);
  void BindTexture3D(const plTempHashedString& sSlotName, plGALResourceViewHandle hResourceView);
  void BindTextureCube(const plTempHashedString& sSlotName, plGALResourceViewHandle hResourceView);

  /// Binds a read+write texture or buffer
  void BindUAV(const plTempHashedString& sSlotName, plGALUnorderedAccessViewHandle hUnorderedAccessViewHandle);

  void BindSamplerState(const plTempHashedString& sSlotName, plGALSamplerStateHandle hSamplerSate);

  void BindBuffer(const plTempHashedString& sSlotName, plGALResourceViewHandle hResourceView);

  void BindConstantBuffer(const plTempHashedString& sSlotName, plGALBufferHandle hConstantBuffer);
  void BindConstantBuffer(const plTempHashedString& sSlotName, plConstantBufferStorageHandle hConstantBufferStorage);

  /// \brief Sets push constants to the given data block.
  /// Note that for platforms that don't support push constants, this is emulated via a constant buffer. Thus, a slot name must be provided as well which matches the name of the BEGIN_PUSH_CONSTANTS block in the shader.
  /// \param sSlotName Name of the BEGIN_PUSH_CONSTANTS block in the shader.
  /// \param data Data of the push constants. If more than 128 bytes, plGALDeviceCapabilities::m_uiMaxPushConstantsSize should be checked to ensure the data block is not too big for the platform.
  void SetPushConstants(const plTempHashedString& sSlotName, plArrayPtr<const plUInt8> data);

  /// Templated version of SetPushConstants.
  /// \tparam T Type of the push constants struct.
  /// \param sSlotName Name of the BEGIN_PUSH_CONSTANTS block in the shader.
  /// \param constants Instance of type T that contains the push constants.
  template <typename T>
  PL_ALWAYS_INLINE void SetPushConstants(const plTempHashedString& sSlotName, const T& constants)
  {
    SetPushConstants(sSlotName, plArrayPtr<const plUInt8>(reinterpret_cast<const plUInt8*>(&constants), sizeof(T)));
  }

  /// \brief Sets the currently active shader on the given render context.
  ///
  /// This function has no effect until the next draw or dispatch call on the context.
  void BindShader(const plShaderResourceHandle& hShader, plBitflags<plShaderBindFlags> flags = plShaderBindFlags::Default);

  void BindMeshBuffer(const plDynamicMeshBufferResourceHandle& hDynamicMeshBuffer);
  void BindMeshBuffer(const plMeshBufferResourceHandle& hMeshBuffer);
  void BindMeshBuffer(plGALBufferHandle hVertexBuffer, plGALBufferHandle hIndexBuffer, const plVertexDeclarationInfo* pVertexDeclarationInfo, plGALPrimitiveTopology::Enum topology, plUInt32 uiPrimitiveCount, plGALBufferHandle hVertexBuffer2 = {}, plGALBufferHandle hVertexBuffer3 = {}, plGALBufferHandle hVertexBuffer4 = {});
  PL_ALWAYS_INLINE void BindNullMeshBuffer(plGALPrimitiveTopology::Enum topology, plUInt32 uiPrimitiveCount)
  {
    BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, topology, uiPrimitiveCount);
  }

  plResult DrawMeshBuffer(plUInt32 uiPrimitiveCount = 0xFFFFFFFF, plUInt32 uiFirstPrimitive = 0, plUInt32 uiInstanceCount = 1);

  plResult Dispatch(plUInt32 uiThreadGroupCountX, plUInt32 uiThreadGroupCountY = 1, plUInt32 uiThreadGroupCountZ = 1);
  plResult DispatchIndirect(plGALBufferHandle hIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes);


  plResult ApplyContextStates(bool bForce = false);
  void ResetContextState();

  plGlobalConstants& WriteGlobalConstants();
  const plGlobalConstants& ReadGlobalConstants() const;

  /// \brief Sets the texture filter mode that is used by default for texture resources.
  ///
  /// The built in default is Anisotropic 4x.
  /// If the default setting is changed, already loaded textures might not adjust.
  /// Nearest filtering is not allowed as a default filter.
  void SetDefaultTextureFilter(plTextureFilterSetting::Enum filter);

  /// \brief Returns the texture filter mode that is used by default for textures.
  plTextureFilterSetting::Enum GetDefaultTextureFilter() const { return m_DefaultTextureFilter; }

  /// \brief Returns the 'fixed' texture filter setting that the combination of default texture filter and given \a configuration defines.
  ///
  /// If \a configuration is set to a fixed filter, that setting is returned.
  /// If it is one of LowestQuality to HighestQuality, the adjusted default filter is returned.
  /// When the default filter is used (with adjustments), the allowed range is Bilinear to Aniso16x, the Nearest filter is never used.
  plTextureFilterSetting::Enum GetSpecificTextureFilter(plTextureFilterSetting::Enum configuration) const;

  /// \brief Set async shader loading. During runtime all shaders should be preloaded so this is off by default.
  void SetAllowAsyncShaderLoading(bool bAllow);

  /// \brief Returns async shader loading. During runtime all shaders should be preloaded so this is off by default.
  bool GetAllowAsyncShaderLoading();


  // Static Functions
public:
  // Constant buffer storage handling
  template <typename T>
  PL_ALWAYS_INLINE static plConstantBufferStorageHandle CreateConstantBufferStorage()
  {
    return CreateConstantBufferStorage(sizeof(T));
  }

  template <typename T>
  PL_FORCE_INLINE static plConstantBufferStorageHandle CreateConstantBufferStorage(plConstantBufferStorage<T>*& out_pStorage)
  {
    plConstantBufferStorageBase* pStorage;
    plConstantBufferStorageHandle hStorage = CreateConstantBufferStorage(sizeof(T), pStorage);
    out_pStorage = static_cast<plConstantBufferStorage<T>*>(pStorage);
    return hStorage;
  }

  PL_FORCE_INLINE static plConstantBufferStorageHandle CreateConstantBufferStorage(plUInt32 uiSizeInBytes)
  {
    plConstantBufferStorageBase* pStorage;
    return CreateConstantBufferStorage(uiSizeInBytes, pStorage);
  }

  static plConstantBufferStorageHandle CreateConstantBufferStorage(plUInt32 uiSizeInBytes, plConstantBufferStorageBase*& out_pStorage);
  static void DeleteConstantBufferStorage(plConstantBufferStorageHandle hStorage);

  template <typename T>
  PL_FORCE_INLINE static bool TryGetConstantBufferStorage(plConstantBufferStorageHandle hStorage, plConstantBufferStorage<T>*& out_pStorage)
  {
    plConstantBufferStorageBase* pStorage = nullptr;
    bool bResult = TryGetConstantBufferStorage(hStorage, pStorage);
    out_pStorage = static_cast<plConstantBufferStorage<T>*>(pStorage);
    return bResult;
  }

  static bool TryGetConstantBufferStorage(plConstantBufferStorageHandle hStorage, plConstantBufferStorageBase*& out_pStorage);

  template <typename T>
  PL_FORCE_INLINE static T* GetConstantBufferData(plConstantBufferStorageHandle hStorage)
  {
    plConstantBufferStorage<T>* pStorage = nullptr;
    if (TryGetConstantBufferStorage(hStorage, pStorage))
    {
      return &(pStorage->GetDataForWriting());
    }

    return nullptr;
  }

  // Default sampler state
  static plGALSamplerStateCreationDescription GetDefaultSamplerState(plBitflags<plDefaultSamplerFlags> flags);

private:
  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, RendererContext);

  static void LoadBuiltinShader(plShaderUtils::plBuiltinShaderType type, plShaderUtils::plBuiltinShader& out_shader);
  static void RegisterImmutableSamplers();
  static void OnEngineStartup();
  static void OnEngineShutdown();

private:
  Statistics m_Statistics;
  plBitflags<plRenderContextFlags> m_StateFlags;
  plShaderResourceHandle m_hActiveShader;
  plGALShaderHandle m_hActiveGALShader;

  plHashTable<plHashedString, plHashedString> m_PermutationVariables;
  plMaterialResourceHandle m_hNewMaterial;
  plMaterialResourceHandle m_hMaterial;

  plShaderPermutationResourceHandle m_hActiveShaderPermutation;

  plBitflags<plShaderBindFlags> m_ShaderBindFlags;

  plGALBufferHandle m_hVertexBuffers[4];
  plGALBufferHandle m_hIndexBuffer;
  const plVertexDeclarationInfo* m_pVertexDeclarationInfo;
  plGALPrimitiveTopology::Enum m_Topology;
  plUInt32 m_uiMeshBufferPrimitiveCount;
  plEnum<plTextureFilterSetting> m_DefaultTextureFilter;
  bool m_bAllowAsyncShaderLoading;
  bool m_bStereoRendering = false;

  plHashTable<plUInt64, plGALResourceViewHandle> m_BoundTextures2D;
  plHashTable<plUInt64, plGALResourceViewHandle> m_BoundTextures3D;
  plHashTable<plUInt64, plGALResourceViewHandle> m_BoundTexturesCube;
  plHashTable<plUInt64, plGALUnorderedAccessViewHandle> m_BoundUAVs;
  plHashTable<plUInt64, plGALSamplerStateHandle> m_BoundSamplers;
  plHashTable<plUInt64, plGALResourceViewHandle> m_BoundBuffer;
  plGALSamplerStateHandle m_hFallbackSampler;

  struct BoundConstantBuffer
  {
    PL_DECLARE_POD_TYPE();

    BoundConstantBuffer() = default;
    BoundConstantBuffer(plGALBufferHandle hConstantBuffer)
      : m_hConstantBuffer(hConstantBuffer)
    {
    }
    BoundConstantBuffer(plConstantBufferStorageHandle hConstantBufferStorage)
      : m_hConstantBufferStorage(hConstantBufferStorage)
    {
    }

    plGALBufferHandle m_hConstantBuffer;
    plConstantBufferStorageHandle m_hConstantBufferStorage;
  };

  plHashTable<plUInt64, BoundConstantBuffer> m_BoundConstantBuffers;

  plConstantBufferStorageHandle m_hGlobalConstantBufferStorage;
  plConstantBufferStorageHandle m_hPushConstantsStorage;

  struct ShaderVertexDecl
  {
    plGALShaderHandle m_hShader;
    plUInt32 m_uiVertexDeclarationHash;

    PL_FORCE_INLINE bool operator<(const ShaderVertexDecl& rhs) const
    {
      if (m_hShader < rhs.m_hShader)
        return true;
      if (rhs.m_hShader < m_hShader)
        return false;
      return m_uiVertexDeclarationHash < rhs.m_uiVertexDeclarationHash;
    }

    PL_FORCE_INLINE bool operator==(const ShaderVertexDecl& rhs) const
    {
      return (m_hShader == rhs.m_hShader && m_uiVertexDeclarationHash == rhs.m_uiVertexDeclarationHash);
    }
  };

  static plResult BuildVertexDeclaration(plGALShaderHandle hShader, const plVertexDeclarationInfo& decl, plGALVertexDeclarationHandle& out_Declaration);

  static plMap<ShaderVertexDecl, plGALVertexDeclarationHandle> s_GALVertexDeclarations;

  static plMutex s_ConstantBufferStorageMutex;
  static plIdTable<plConstantBufferStorageId, plConstantBufferStorageBase*> s_ConstantBufferStorageTable;
  static plMap<plUInt32, plDynamicArray<plConstantBufferStorageBase*>> s_FreeConstantBufferStorage;

private: // Per Renderer States
  friend RenderingScope;
  friend ComputeScope;
  PL_ALWAYS_INLINE void EndCommandEncoder(plGALRenderCommandEncoder*) { EndRendering(); }
  PL_ALWAYS_INLINE void EndCommandEncoder(plGALComputeCommandEncoder*) { EndCompute(); }

  plGALPass* m_pGALPass = nullptr;
  plGALCommandEncoder* m_pGALCommandEncoder = nullptr;
  bool m_bCompute = false;

  // Member Functions
  void UploadConstants();

  void SetShaderPermutationVariableInternal(const plHashedString& sName, const plHashedString& sValue);
  void BindShaderInternal(const plShaderResourceHandle& hShader, plBitflags<plShaderBindFlags> flags);
  plShaderPermutationResource* ApplyShaderState();
  plMaterialResource* ApplyMaterialState();
  void ApplyConstantBufferBindings(const plGALShader* pShader);
  void ApplyTextureBindings(const plGALShader* pShader);
  void ApplyUAVBindings(const plGALShader* pShader);
  void ApplySamplerBindings(const plGALShader* pShader);
  void ApplyBufferBindings(const plGALShader* pShader);
};
