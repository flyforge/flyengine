#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RmlUiPlugin/Implementation/Extractor.h>

namespace plRmlUiInternal
{
  Extractor::Extractor()
  {
    m_hFallbackTexture = plResourceManager::LoadResource<plTexture2DResource>("White.color");

    plGALDevice::s_Events.AddEventHandler(plMakeDelegate(&Extractor::EndFrame, this));
  }

  Extractor::~Extractor()
  {
    plGALDevice::s_Events.RemoveEventHandler(plMakeDelegate(&Extractor::EndFrame, this));

    for (auto it = m_CompiledGeometry.GetIterator(); it.IsValid(); ++it)
    {
      FreeReleasedGeometry(it.Id());
    }
  }

  void Extractor::RenderGeometry(Rml::Vertex* pVertices, int iNum_vertices, int* pIndices, int iNum_indices, Rml::TextureHandle hTexture, const Rml::Vector2f& translation)
  {
    // Should never be called since we are using compiled geometry
    PL_ASSERT_NOT_IMPLEMENTED;
  }

  Rml::CompiledGeometryHandle Extractor::CompileGeometry(Rml::Vertex* pVertices, int iNum_vertices, int* pIndices, int iNum_indices, Rml::TextureHandle hTexture)
  {
    CompiledGeometry geometry;
    geometry.m_uiTriangleCount = iNum_indices / 3;

    // vertices
    {
      plDynamicArray<Vertex> vertexStorage(plFrameAllocator::GetCurrentAllocator());
      vertexStorage.SetCountUninitialized(iNum_vertices);

      for (plUInt32 i = 0; i < vertexStorage.GetCount(); ++i)
      {
        auto& srcVertex = pVertices[i];
        auto& destVertex = vertexStorage[i];
        destVertex.m_Position = plVec3(srcVertex.position.x, srcVertex.position.y, 0);
        destVertex.m_TexCoord = plVec2(srcVertex.tex_coord.x, srcVertex.tex_coord.y);
        destVertex.m_Color = reinterpret_cast<const plColorGammaUB&>(srcVertex.colour);
      }

      plGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(Vertex);
      desc.m_uiTotalSize = vertexStorage.GetCount() * desc.m_uiStructSize;
      desc.m_BufferType = plGALBufferType::VertexBuffer;

      geometry.m_hVertexBuffer = plGALDevice::GetDefaultDevice()->CreateBuffer(desc, vertexStorage.GetByteArrayPtr());
    }

    // indices
    {
      plGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(plUInt32);
      desc.m_uiTotalSize = iNum_indices * desc.m_uiStructSize;
      desc.m_BufferType = plGALBufferType::IndexBuffer;

      geometry.m_hIndexBuffer = plGALDevice::GetDefaultDevice()->CreateBuffer(desc, plMakeArrayPtr(pIndices, iNum_indices).ToByteArray());
    }

    // texture
    {
      plTexture2DResourceHandle* phTexture = nullptr;
      if (m_Textures.TryGetValue(TextureId::FromRml(hTexture), phTexture))
      {
        geometry.m_hTexture = *phTexture;
      }
      else
      {
        geometry.m_hTexture = m_hFallbackTexture;
      }
    }

    return m_CompiledGeometry.Insert(std::move(geometry)).ToRml();
  }

  void Extractor::RenderCompiledGeometry(Rml::CompiledGeometryHandle hGeometry_handle, const Rml::Vector2f& translation)
  {
    auto& batch = m_Batches.ExpandAndGetRef();

    PL_VERIFY(m_CompiledGeometry.TryGetValue(GeometryId::FromRml(hGeometry_handle), batch.m_CompiledGeometry), "Invalid compiled geometry");

    plMat4 offsetMat = plMat4::MakeTranslation(m_vOffset.GetAsVec3(0));

    batch.m_Transform = offsetMat * m_mTransform;
    batch.m_Translation = plVec2(translation.x, translation.y);

    batch.m_ScissorRect = m_ScissorRect;
    batch.m_bEnableScissorRect = m_bEnableScissorRect;
    batch.m_bTransformScissorRect = (m_bEnableScissorRect && m_mTransform.IsIdentity() == false);

    if (!batch.m_bTransformScissorRect)
    {
      batch.m_ScissorRect.x += m_vOffset.x;
      batch.m_ScissorRect.y += m_vOffset.y;
    }
  }

  void Extractor::ReleaseCompiledGeometry(Rml::CompiledGeometryHandle hGeometry_handle)
  {
    m_ReleasedCompiledGeometry.PushBack({plRenderWorld::GetFrameCounter(), GeometryId::FromRml(hGeometry_handle)});
  }

  void Extractor::EnableScissorRegion(bool bEnable)
  {
    m_bEnableScissorRect = bEnable;
  }

  void Extractor::SetScissorRegion(int x, int y, int iWidth, int iHeight)
  {
    m_ScissorRect = plRectFloat(static_cast<float>(x), static_cast<float>(y), static_cast<float>(iWidth), static_cast<float>(iHeight));
  }

  bool Extractor::LoadTexture(Rml::TextureHandle& ref_hTexture_handle, Rml::Vector2i& ref_texture_dimensions, const Rml::String& sSource)
  {
    plTexture2DResourceHandle hTexture = plResourceManager::LoadResource<plTexture2DResource>(sSource.c_str());

    plResourceLock<plTexture2DResource> pTexture(hTexture, plResourceAcquireMode::BlockTillLoaded);
    if (pTexture.GetAcquireResult() == plResourceAcquireResult::Final)
    {
      ref_hTexture_handle = m_Textures.Insert(hTexture).ToRml();
      ref_texture_dimensions = Rml::Vector2i(pTexture->GetWidth(), pTexture->GetHeight());

      return true;
    }

    return false;
  }

  bool Extractor::GenerateTexture(Rml::TextureHandle& ref_hTexture_handle, const Rml::byte* pSource, const Rml::Vector2i& source_dimensions)
  {
    plUInt32 uiWidth = source_dimensions.x;
    plUInt32 uiHeight = source_dimensions.y;
    plUInt32 uiSizeInBytes = uiWidth * uiHeight * 4;

    plUInt64 uiHash = plHashingUtils::xxHash64(pSource, uiSizeInBytes);

    plStringBuilder sTextureName;
    sTextureName.SetFormat("RmlUiGeneratedTexture_{}x{}_{}", uiWidth, uiHeight, uiHash);

    plTexture2DResourceHandle hTexture = plResourceManager::GetExistingResource<plTexture2DResource>(sTextureName);

    if (!hTexture.IsValid())
    {
      plGALSystemMemoryDescription memoryDesc;
      memoryDesc.m_pData = const_cast<Rml::byte*>(pSource);
      memoryDesc.m_uiRowPitch = uiWidth * 4;
      memoryDesc.m_uiSlicePitch = uiSizeInBytes;

      plTexture2DResourceDescriptor desc;
      desc.m_DescGAL.m_uiWidth = uiWidth;
      desc.m_DescGAL.m_uiHeight = uiHeight;
      desc.m_DescGAL.m_Format = plGALResourceFormat::RGBAUByteNormalized;
      desc.m_InitialContent = plMakeArrayPtr(&memoryDesc, 1);

      hTexture = plResourceManager::GetOrCreateResource<plTexture2DResource>(sTextureName, std::move(desc));
    }

    ref_hTexture_handle = m_Textures.Insert(hTexture).ToRml();
    return true;
  }

  void Extractor::ReleaseTexture(Rml::TextureHandle hTexture_handle)
  {
    PL_VERIFY(m_Textures.Remove(TextureId::FromRml(hTexture_handle)), "Invalid texture handle");
  }

  void Extractor::SetTransform(const Rml::Matrix4f* pTransform)
  {
    if (pTransform != nullptr)
    {
      constexpr bool bColumnMajor = std::is_same<Rml::Matrix4f, Rml::ColumnMajorMatrix4f>::value;

      if (bColumnMajor)
        m_mTransform = plMat4::MakeFromColumnMajorArray(pTransform->data());
      else
        m_mTransform = plMat4::MakeFromRowMajorArray(pTransform->data());
    }
    else
    {
      m_mTransform.SetIdentity();
    }
  }

  void Extractor::BeginExtraction(const plVec2I32& vOffset)
  {
    m_vOffset = plVec2(static_cast<float>(vOffset.x), static_cast<float>(vOffset.y));
    m_mTransform = plMat4::MakeIdentity();

    m_Batches.Clear();
  }

  void Extractor::EndExtraction() {}

  plRenderData* Extractor::GetRenderData()
  {
    if (m_Batches.IsEmpty() == false)
    {
      plRmlUiRenderData* pRenderData = PL_NEW(plFrameAllocator::GetCurrentAllocator(), plRmlUiRenderData, plFrameAllocator::GetCurrentAllocator());
      pRenderData->m_GlobalTransform.SetIdentity();
      pRenderData->m_GlobalBounds = plBoundingBoxSphere::MakeInvalid();
      pRenderData->m_Batches = m_Batches;

      return pRenderData;
    }

    return nullptr;
  }

  void Extractor::EndFrame(const plGALDeviceEvent& e)
  {
    if (e.m_Type != plGALDeviceEvent::BeforeEndFrame)
      return;

    plUInt64 uiFrameCounter = plRenderWorld::GetFrameCounter();

    while (!m_ReleasedCompiledGeometry.IsEmpty())
    {
      auto& releasedGeometry = m_ReleasedCompiledGeometry.PeekFront();

      if (releasedGeometry.m_uiFrame >= uiFrameCounter)
        break;

      FreeReleasedGeometry(releasedGeometry.m_Id);

      m_CompiledGeometry.Remove(releasedGeometry.m_Id);
      m_ReleasedCompiledGeometry.PopFront();
    }
  }

  void Extractor::FreeReleasedGeometry(GeometryId id)
  {
    CompiledGeometry* pGeometry = nullptr;
    if (!m_CompiledGeometry.TryGetValue(id, pGeometry))
      return;

    plGALDevice::GetDefaultDevice()->DestroyBuffer(pGeometry->m_hVertexBuffer);
    pGeometry->m_hVertexBuffer.Invalidate();

    plGALDevice::GetDefaultDevice()->DestroyBuffer(pGeometry->m_hIndexBuffer);
    pGeometry->m_hIndexBuffer.Invalidate();

    pGeometry->m_hTexture.Invalidate();
  }

} // namespace plRmlUiInternal
