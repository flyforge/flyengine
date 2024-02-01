#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>

namespace plRmlUiInternal
{
  struct GeometryId : public plGenericId<24, 8>
  {
    using plGenericId::plGenericId;

    static GeometryId FromRml(Rml::CompiledGeometryHandle hGeometry) { return GeometryId(static_cast<plUInt32>(hGeometry)); }

    Rml::CompiledGeometryHandle ToRml() const { return m_Data; }
  };

  struct TextureId : public plGenericId<24, 8>
  {
    using plGenericId::plGenericId;

    static TextureId FromRml(Rml::TextureHandle hTexture) { return TextureId(static_cast<plUInt32>(hTexture)); }

    Rml::TextureHandle ToRml() const { return m_Data; }
  };

  //////////////////////////////////////////////////////////////////////////

  class Extractor final : public Rml::RenderInterface
  {
  public:
    Extractor();
    virtual ~Extractor();

    virtual void RenderGeometry(Rml::Vertex* pVertices, int iNum_vertices, int* pIndices, int iNum_indices, Rml::TextureHandle hTexture, const Rml::Vector2f& translation) override;

    virtual Rml::CompiledGeometryHandle CompileGeometry(Rml::Vertex* pVertices, int iNum_vertices, int* pIndices, int iNum_indices, Rml::TextureHandle hTexture) override;
    virtual void RenderCompiledGeometry(Rml::CompiledGeometryHandle hGeometry_handle, const Rml::Vector2f& translation) override;
    virtual void ReleaseCompiledGeometry(Rml::CompiledGeometryHandle hGeometry_handle) override;

    virtual void EnableScissorRegion(bool bEnable) override;
    virtual void SetScissorRegion(int x, int y, int iWidth, int iHeight) override;

    virtual bool LoadTexture(Rml::TextureHandle& ref_hTexture_handle, Rml::Vector2i& ref_texture_dimensions, const Rml::String& sSource) override;
    virtual bool GenerateTexture(Rml::TextureHandle& ref_hTexture_handle, const Rml::byte* pSource, const Rml::Vector2i& source_dimensions) override;
    virtual void ReleaseTexture(Rml::TextureHandle hTexture_handle) override;

    virtual void SetTransform(const Rml::Matrix4f* pTransform) override;

    void BeginExtraction(const plVec2I32& vOffset);
    void EndExtraction();

    plRenderData* GetRenderData();

  private:
    void EndFrame(const plGALDeviceEvent& e);
    void FreeReleasedGeometry(GeometryId id);

    plIdTable<GeometryId, CompiledGeometry> m_CompiledGeometry;

    struct ReleasedGeometry
    {
      plUInt64 m_uiFrame;
      GeometryId m_Id;
    };

    plDeque<ReleasedGeometry> m_ReleasedCompiledGeometry;

    plIdTable<TextureId, plTexture2DResourceHandle> m_Textures;
    plTexture2DResourceHandle m_hFallbackTexture;

    plVec2 m_vOffset = plVec2::MakeZero();

    plMat4 m_mTransform = plMat4::MakeIdentity();
    plRectFloat m_ScissorRect = plRectFloat(0, 0);
    bool m_bEnableScissorRect = false;

    plDynamicArray<Batch> m_Batches;
  };
} // namespace plRmlUiInternal
