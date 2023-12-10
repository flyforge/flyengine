#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/World.h>
#include <Foundation/Containers/HybridArray.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Debug/SimpleASCIIFont.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

//////////////////////////////////////////////////////////////////////////

plDebugRendererContext::plDebugRendererContext(const plWorld* pWorld)
  : m_uiId(pWorld != nullptr ? pWorld->GetIndex() : 0)
{
}

plDebugRendererContext::plDebugRendererContext(const plViewHandle& hView)
  : m_uiId(hView.GetInternalID().m_Data)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plDebugTextHAlign, 1)
  PLASMA_ENUM_CONSTANTS(plDebugTextHAlign::Left, plDebugTextHAlign::Center, plDebugTextHAlign::Right)
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plDebugTextVAlign, 1)
  PLASMA_ENUM_CONSTANTS(plDebugTextVAlign::Top, plDebugTextVAlign::Center, plDebugTextVAlign::Bottom)
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plDebugTextPlacement, 1)
  PLASMA_ENUM_CONSTANTS(plDebugTextPlacement::TopLeft, plDebugTextPlacement::TopCenter, plDebugTextPlacement::TopRight)
  PLASMA_ENUM_CONSTANTS(plDebugTextPlacement::BottomLeft, plDebugTextPlacement::BottomCenter, plDebugTextPlacement::BottomRight)
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

namespace
{
  struct alignas(16) Vertex
  {
    plVec3 m_position;
    plColorLinearUB m_color;
  };

  PLASMA_CHECK_AT_COMPILETIME(sizeof(Vertex) == 16);

  struct alignas(16) TexVertex
  {
    plVec3 m_position;
    plColorLinearUB m_color;
    plVec2 m_texCoord;
    float padding[2];
  };

  PLASMA_CHECK_AT_COMPILETIME(sizeof(TexVertex) == 32);

  struct alignas(16) BoxData
  {
    plShaderTransform m_transform;
    plColor m_color;
  };

  PLASMA_CHECK_AT_COMPILETIME(sizeof(BoxData) == 64);

  struct alignas(16) GlyphData
  {
    plVec2 m_topLeftCorner;
    plColorLinearUB m_color;
    plUInt16 m_glyphIndex;
    plUInt16 m_sizeInPixel;
  };

  PLASMA_CHECK_AT_COMPILETIME(sizeof(GlyphData) == 16);

  struct TextLineData2D
  {
    plString m_text;
    plVec2 m_topLeftCorner;
    plColorLinearUB m_color;
    plUInt32 m_uiSizeInPixel;
  };

  struct TextLineData3D : public TextLineData2D
  {
    plVec3 m_position;
  };

  struct InfoTextData
  {
    plString m_group;
    plString m_text;
    plColor m_color;
  };

  struct PerContextData
  {
    plDynamicArray<Vertex, plAlignedAllocatorWrapper> m_lineVertices;
    plDynamicArray<Vertex, plAlignedAllocatorWrapper> m_triangleVertices;
    plDynamicArray<Vertex, plAlignedAllocatorWrapper> m_triangle2DVertices;
    plDynamicArray<Vertex, plAlignedAllocatorWrapper> m_line2DVertices;
    plDynamicArray<BoxData, plAlignedAllocatorWrapper> m_lineBoxes;
    plDynamicArray<BoxData, plAlignedAllocatorWrapper> m_solidBoxes;
    plMap<plGALResourceViewHandle, plDynamicArray<TexVertex, plAlignedAllocatorWrapper>> m_texTriangle2DVertices;
    plMap<plGALResourceViewHandle, plDynamicArray<TexVertex, plAlignedAllocatorWrapper>> m_texTriangle3DVertices;

    plDynamicArray<InfoTextData> m_infoTextData[(int)plDebugTextPlacement::ENUM_COUNT];
    plDynamicArray<TextLineData2D> m_textLines2D;
    plDynamicArray<TextLineData3D> m_textLines3D;
    plDynamicArray<GlyphData, plAlignedAllocatorWrapper> m_glyphs;
  };

  struct DoubleBufferedPerContextData
  {
    DoubleBufferedPerContextData()
    {
      m_uiLastRenderedFrame = 0;
      m_pData[0] = nullptr;
      m_pData[1] = nullptr;
    }

    plUInt64 m_uiLastRenderedFrame;
    plUniquePtr<PerContextData> m_pData[2];
  };

  static plHashTable<plDebugRendererContext, DoubleBufferedPerContextData> s_PerContextData;
  static plMutex s_Mutex;

  static PerContextData& GetDataForExtraction(const plDebugRendererContext& context)
  {
    DoubleBufferedPerContextData& doubleBufferedData = s_PerContextData[context];

    const plUInt32 uiDataIndex = plRenderWorld::IsRenderingThread() && (doubleBufferedData.m_uiLastRenderedFrame != plRenderWorld::GetFrameCounter()) ? plRenderWorld::GetDataIndexForRendering() : plRenderWorld::GetDataIndexForExtraction();

    plUniquePtr<PerContextData>& pData = doubleBufferedData.m_pData[uiDataIndex];
    if (pData == nullptr)
    {
      doubleBufferedData.m_pData[uiDataIndex] = PLASMA_DEFAULT_NEW(PerContextData);
    }

    return *pData;
  }

  static void ClearRenderData()
  {
    PLASMA_LOCK(s_Mutex);

    for (auto it = s_PerContextData.GetIterator(); it.IsValid(); ++it)
    {
      PerContextData* pData = it.Value().m_pData[plRenderWorld::GetDataIndexForRendering()].Borrow();
      if (pData)
      {
        pData->m_lineVertices.Clear();
        pData->m_line2DVertices.Clear();
        pData->m_lineBoxes.Clear();
        pData->m_solidBoxes.Clear();
        pData->m_triangleVertices.Clear();
        pData->m_triangle2DVertices.Clear();
        pData->m_texTriangle2DVertices.Clear();
        pData->m_texTriangle3DVertices.Clear();
        pData->m_textLines2D.Clear();
        pData->m_textLines3D.Clear();

        for (plUInt32 i = 0; i < (plUInt32)plDebugTextPlacement::ENUM_COUNT; ++i)
        {
          pData->m_infoTextData[i].Clear();
        }
      }
    }
  }

  static void OnRenderEvent(const plRenderWorldRenderEvent& e)
  {
    if (e.m_Type == plRenderWorldRenderEvent::Type::EndRender)
    {
      ClearRenderData();
    }
  }

  struct BufferType
  {
    enum Enum
    {
      Lines,
      LineBoxes,
      SolidBoxes,
      Triangles3D,
      Triangles2D,
      TexTriangles2D,
      TexTriangles3D,
      Glyphs,
      Lines2D,

      Count
    };
  };

  static plGALBufferHandle s_hDataBuffer[BufferType::Count];

  static plMeshBufferResourceHandle s_hLineBoxMeshBuffer;
  static plMeshBufferResourceHandle s_hSolidBoxMeshBuffer;
  static plVertexDeclarationInfo s_VertexDeclarationInfo;
  static plVertexDeclarationInfo s_TexVertexDeclarationInfo;
  static plTexture2DResourceHandle s_hDebugFontTexture;

  static plShaderResourceHandle s_hDebugGeometryShader;
  static plShaderResourceHandle s_hDebugPrimitiveShader;
  static plShaderResourceHandle s_hDebugTexturedPrimitiveShader;
  static plShaderResourceHandle s_hDebugTextShader;

  enum
  {
    DEBUG_BUFFER_SIZE = 1024 * 256,
    BOXES_PER_BATCH = DEBUG_BUFFER_SIZE / sizeof(BoxData),
    LINE_VERTICES_PER_BATCH = DEBUG_BUFFER_SIZE / sizeof(Vertex),
    TRIANGLE_VERTICES_PER_BATCH = (DEBUG_BUFFER_SIZE / sizeof(Vertex) / 3) * 3,
    TEX_TRIANGLE_VERTICES_PER_BATCH = (DEBUG_BUFFER_SIZE / sizeof(TexVertex) / 3) * 3,
    GLYPHS_PER_BATCH = DEBUG_BUFFER_SIZE / sizeof(GlyphData),
  };

  static void CreateDataBuffer(BufferType::Enum bufferType, plUInt32 uiStructSize)
  {
    if (s_hDataBuffer[bufferType].IsInvalidated())
    {
      plGALBufferCreationDescription desc;
      desc.m_uiStructSize = uiStructSize;
      desc.m_uiTotalSize = DEBUG_BUFFER_SIZE;
      desc.m_BufferType = plGALBufferType::Generic;
      desc.m_bUseAsStructuredBuffer = true;
      desc.m_bAllowShaderResourceView = true;
      desc.m_ResourceAccess.m_bImmutable = false;

      s_hDataBuffer[bufferType] = plGALDevice::GetDefaultDevice()->CreateBuffer(desc);
    }
  }

  static void CreateVertexBuffer(BufferType::Enum bufferType, plUInt32 uiVertexSize)
  {
    if (s_hDataBuffer[bufferType].IsInvalidated())
    {
      plGALBufferCreationDescription desc;
      desc.m_uiStructSize = uiVertexSize;
      desc.m_uiTotalSize = DEBUG_BUFFER_SIZE;
      desc.m_BufferType = plGALBufferType::VertexBuffer;
      desc.m_ResourceAccess.m_bImmutable = false;

      s_hDataBuffer[bufferType] = plGALDevice::GetDefaultDevice()->CreateBuffer(desc);
    }
  }

  static void DestroyBuffer(BufferType::Enum bufferType)
  {
    plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

    if (!s_hDataBuffer[bufferType].IsInvalidated())
    {
      pDevice->DestroyBuffer(s_hDataBuffer[bufferType]);

      s_hDataBuffer[bufferType].Invalidate();
    }
  }

  template <typename AddFunc>
  static plUInt32 AddTextLines(const plDebugRendererContext& context, const plFormatString& text0, const plVec2I32& vPositionInPixel, float fSizeInPixel, plDebugTextHAlign::Enum horizontalAlignment, plDebugTextVAlign::Enum verticalAlignment, AddFunc func)
  {
    if (text0.IsEmpty())
      return 0;

    plStringBuilder tmp;
    plStringView text = text0.GetText(tmp);

    plHybridArray<plStringView, 8> lines;
    plUInt32 maxLineLength = 0;

    plHybridArray<plUInt32, 8> maxColumWidth;
    bool isTabular = false;

    plStringBuilder sb;
    if (text.FindSubString("\n"))
    {
      sb = text;
      sb.Split(true, lines, "\n");

      for (auto& line : lines)
      {
        plUInt32 uiColIdx = 0;

        const char* colPtrCur = line.GetStartPointer();

        while (const char* colPtrNext = line.FindSubString("\t", colPtrCur))
        {
          isTabular = true;

          const plUInt32 colLen = plMath::RoundUp(1 + static_cast<plUInt32>(colPtrNext - colPtrCur), 4);

          maxColumWidth.EnsureCount(uiColIdx + 1);
          maxColumWidth[uiColIdx] = plMath::Max(maxColumWidth[uiColIdx], colLen);

          colPtrCur = colPtrNext + 1;
          ++uiColIdx;
        }

        // length of the last column (that wasn't counted)
        maxLineLength = plMath::Max(maxLineLength, plStringUtils::GetStringElementCount(colPtrCur, line.GetEndPointer()));
      }

      for (plUInt32 columnWidth : maxColumWidth)
      {
        maxLineLength += columnWidth;
      }
    }
    else
    {
      lines.PushBack(text);
      maxLineLength = text.GetElementCount();
      maxColumWidth.PushBack(maxLineLength);
    }

    // Glyphs only use 8x10 pixels in their 16x16 pixel block, thus we don't advance by full size here.
    const float fGlyphWidth = plMath::Ceil(fSizeInPixel * (8.0f / 16.0f));
    const float fLineHeight = plMath::Ceil(fSizeInPixel * (20.0f / 16.0f));

    float screenPosX = (float)vPositionInPixel.x;
    if (horizontalAlignment == plDebugTextHAlign::Right)
      screenPosX -= maxLineLength * fGlyphWidth;

    float screenPosY = (float)vPositionInPixel.y;
    if (verticalAlignment == plDebugTextVAlign::Center)
      screenPosY -= plMath::Ceil(lines.GetCount() * fLineHeight * 0.5f);
    else if (verticalAlignment == plDebugTextVAlign::Bottom)
      screenPosY -= lines.GetCount() * fLineHeight;

    {
      PLASMA_LOCK(s_Mutex);

      auto& data = GetDataForExtraction(context);

      plVec2 currentPos(screenPosX, screenPosY);

      for (plStringView line : lines)
      {
        currentPos.x = screenPosX;
        if (horizontalAlignment == plDebugTextHAlign::Center)
          currentPos.x -= plMath::Ceil(line.GetElementCount() * fGlyphWidth * 0.5f);

        if (isTabular)
        {
          plUInt32 uiColIdx = 0;

          const char* colPtrCur = line.GetStartPointer();

          plUInt32 addWidth = 0;

          while (const char* colPtrNext = line.FindSubString("\t", colPtrCur))
          {
            const plVec2 tabOff(addWidth * fGlyphWidth, 0);
            func(data, plStringView(colPtrCur, colPtrNext), currentPos + tabOff);

            addWidth += maxColumWidth[uiColIdx];

            colPtrCur = colPtrNext + 1;
            ++uiColIdx;
          }

          // last column
          {
            const plVec2 tabOff(addWidth * fGlyphWidth, 0);
            func(data, plStringView(colPtrCur, line.GetEndPointer()), currentPos + tabOff);
          }
        }
        else
        {
          func(data, line, currentPos);
        }

        currentPos.y += fLineHeight;
      }
    }

    return lines.GetCount();
  }

  static void AppendGlyphs(plDynamicArray<GlyphData, plAlignedAllocatorWrapper>& ref_glyphs, const TextLineData2D& textLine)
  {
    plVec2 currentPos = textLine.m_topLeftCorner;
    const float fGlyphWidth = plMath::Ceil(textLine.m_uiSizeInPixel * (8.0f / 16.0f));

    for (plUInt32 uiCharacter : textLine.m_text)
    {
      auto& glyphData = ref_glyphs.ExpandAndGetRef();
      glyphData.m_topLeftCorner = currentPos;
      glyphData.m_color = textLine.m_color;
      glyphData.m_glyphIndex = uiCharacter < 128 ? static_cast<plUInt16>(uiCharacter) : 0;
      glyphData.m_sizeInPixel = (plUInt16)textLine.m_uiSizeInPixel;

      currentPos.x += fGlyphWidth;
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // Persistent Items

  struct PersistentCrossData
  {
    float m_fSize;
    plColor m_Color;
    plTransform m_Transform;
    plTime m_Timeout;
  };

  struct PersistentSphereData
  {
    float m_fRadius;
    plColor m_Color;
    plTransform m_Transform;
    plTime m_Timeout;
  };

  struct PersistentBoxData
  {
    plVec3 m_vHalfSize;
    plColor m_Color;
    plTransform m_Transform;
    plTime m_Timeout;
  };

  struct PersistentPerContextData
  {
    plTime m_Now;
    plDeque<PersistentCrossData> m_Crosses;
    plDeque<PersistentSphereData> m_Spheres;
    plDeque<PersistentBoxData> m_Boxes;
  };

  static plHashTable<plDebugRendererContext, PersistentPerContextData> s_PersistentPerContextData;

} // namespace

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DebugRenderer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    plDebugRenderer::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    plDebugRenderer::OnEngineShutdown();
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

// static
void plDebugRenderer::DrawLines(const plDebugRendererContext& context, plArrayPtr<const Line> lines, const plColor& color, const plTransform& transform /*= plTransform::MakeIdentity()*/)
{
  if (lines.IsEmpty())
    return;

  PLASMA_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (auto& line : lines)
  {
    const plVec3* pPositions = &line.m_start;
    const plColor* pColors = &line.m_startColor;

    for (plUInt32 i = 0; i < 2; ++i)
    {
      auto& vertex = data.m_lineVertices.ExpandAndGetRef();
      vertex.m_position = transform.TransformPosition(pPositions[i]);
      vertex.m_color = pColors[i] * color;
    }
  }
}

void plDebugRenderer::Draw2DLines(const plDebugRendererContext& context, plArrayPtr<const Line> lines, const plColor& color)
{
  if (lines.IsEmpty())
    return;

  PLASMA_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (auto& line : lines)
  {
    const plVec3* pPositions = &line.m_start;

    for (plUInt32 i = 0; i < 2; ++i)
    {
      auto& vertex = data.m_line2DVertices.ExpandAndGetRef();
      vertex.m_position = pPositions[i];
      vertex.m_color = color;
    }
  }
}

// static
void plDebugRenderer::DrawCross(const plDebugRendererContext& context, const plVec3& vGlobalPosition, float fLineLength, const plColor& color, const plTransform& transform /*= plTransform::MakeIdentity()*/)
{
  if (fLineLength <= 0.0f)
    return;

  const float fHalfLineLength = fLineLength * 0.5f;
  const plVec3 xAxis = plVec3::MakeAxisX() * fHalfLineLength;
  const plVec3 yAxis = plVec3::MakeAxisY() * fHalfLineLength;
  const plVec3 zAxis = plVec3::MakeAxisZ() * fHalfLineLength;

  PLASMA_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition - xAxis), color});
  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition + xAxis), color});

  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition - yAxis), color});
  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition + yAxis), color});

  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition - zAxis), color});
  data.m_lineVertices.PushBack({transform.TransformPosition(vGlobalPosition + zAxis), color});
}

// static
void plDebugRenderer::DrawLineBox(const plDebugRendererContext& context, const plBoundingBox& box, const plColor& color, const plTransform& transform)
{
  PLASMA_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  auto& boxData = data.m_lineBoxes.ExpandAndGetRef();

  plTransform boxTransform(box.GetCenter(), plQuat::MakeIdentity(), box.GetHalfExtents());

  boxData.m_transform = transform * boxTransform;
  boxData.m_color = color;
}

// static
void plDebugRenderer::DrawLineBoxCorners(const plDebugRendererContext& context, const plBoundingBox& box, float fCornerFraction, const plColor& color, const plTransform& transform)
{
  fCornerFraction = plMath::Clamp(fCornerFraction, 0.0f, 1.0f) * 0.5f;

  plVec3 corners[8];
  box.GetCorners(corners);

  for (plUInt32 i = 0; i < 8; ++i)
  {
    corners[i] = transform * corners[i];
  }

  plVec3 edgeEnds[12];
  edgeEnds[0] = corners[1];  // 0 -> 1
  edgeEnds[1] = corners[3];  // 1 -> 3
  edgeEnds[2] = corners[0];  // 2 -> 0
  edgeEnds[3] = corners[2];  // 3 -> 2
  edgeEnds[4] = corners[5];  // 4 -> 5
  edgeEnds[5] = corners[7];  // 5 -> 7
  edgeEnds[6] = corners[4];  // 6 -> 4
  edgeEnds[7] = corners[6];  // 7 -> 6
  edgeEnds[8] = corners[4];  // 0 -> 4
  edgeEnds[9] = corners[5];  // 1 -> 5
  edgeEnds[10] = corners[6]; // 2 -> 6
  edgeEnds[11] = corners[7]; // 3 -> 7

  Line lines[24];
  for (plUInt32 i = 0; i < 12; ++i)
  {
    plVec3 edgeStart = corners[i % 8];
    plVec3 edgeEnd = edgeEnds[i];
    plVec3 edgeDir = edgeEnd - edgeStart;

    lines[i * 2 + 0].m_start = edgeStart;
    lines[i * 2 + 0].m_end = edgeStart + edgeDir * fCornerFraction;

    lines[i * 2 + 1].m_start = edgeEnd;
    lines[i * 2 + 1].m_end = edgeEnd - edgeDir * fCornerFraction;
  }

  DrawLines(context, lines, color);
}

// static
void plDebugRenderer::DrawLineSphere(const plDebugRendererContext& context, const plBoundingSphere& sphere, const plColor& color, const plTransform& transform /*= plTransform::MakeIdentity()*/)
{
  enum
  {
    NUM_SEGMENTS = 32
  };

  const plVec3 vCenter = sphere.m_vCenter;
  const float fRadius = sphere.m_fRadius;
  const plAngle stepAngle = plAngle::MakeFromDegree(360.0f / (float)NUM_SEGMENTS);

  PLASMA_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (plUInt32 s = 0; s < NUM_SEGMENTS; ++s)
  {
    const float fS1 = (float)s;
    const float fS2 = (float)(s + 1);

    const float fCos1 = plMath::Cos(fS1 * stepAngle);
    const float fCos2 = plMath::Cos(fS2 * stepAngle);

    const float fSin1 = plMath::Sin(fS1 * stepAngle);
    const float fSin2 = plMath::Sin(fS2 * stepAngle);

    data.m_lineVertices.PushBack({transform * (vCenter + plVec3(0.0f, fCos1, fSin1) * fRadius), color});
    data.m_lineVertices.PushBack({transform * (vCenter + plVec3(0.0f, fCos2, fSin2) * fRadius), color});

    data.m_lineVertices.PushBack({transform * (vCenter + plVec3(fCos1, 0.0f, fSin1) * fRadius), color});
    data.m_lineVertices.PushBack({transform * (vCenter + plVec3(fCos2, 0.0f, fSin2) * fRadius), color});

    data.m_lineVertices.PushBack({transform * (vCenter + plVec3(fCos1, fSin1, 0.0f) * fRadius), color});
    data.m_lineVertices.PushBack({transform * (vCenter + plVec3(fCos2, fSin2, 0.0f) * fRadius), color});
  }
}


void plDebugRenderer::DrawLineCapsuleZ(const plDebugRendererContext& context, float fLength, float fRadius, const plColor& color, const plTransform& transform /*= plTransform::MakeIdentity()*/)
{
  enum
  {
    NUM_SEGMENTS = 32,
    NUM_HALF_SEGMENTS = 16,
    NUM_LINES = NUM_SEGMENTS + NUM_SEGMENTS + NUM_SEGMENTS + NUM_SEGMENTS + 4,
  };

  const plAngle stepAngle = plAngle::MakeFromDegree(360.0f / (float)NUM_SEGMENTS);

  Line lines[NUM_LINES];

  const float fOffsetZ = fLength * 0.5f;

  plUInt32 curLine = 0;

  // render 4 straight lines
  lines[curLine].m_start = transform * plVec3(-fRadius, 0, fOffsetZ);
  lines[curLine].m_end = transform * plVec3(-fRadius, 0, -fOffsetZ);
  ++curLine;

  lines[curLine].m_start = transform * plVec3(+fRadius, 0, fOffsetZ);
  lines[curLine].m_end = transform * plVec3(+fRadius, 0, -fOffsetZ);
  ++curLine;

  lines[curLine].m_start = transform * plVec3(0, -fRadius, fOffsetZ);
  lines[curLine].m_end = transform * plVec3(0, -fRadius, -fOffsetZ);
  ++curLine;

  lines[curLine].m_start = transform * plVec3(0, +fRadius, fOffsetZ);
  lines[curLine].m_end = transform * plVec3(0, +fRadius, -fOffsetZ);
  ++curLine;

  // render top and bottom circle
  for (plUInt32 s = 0; s < NUM_SEGMENTS; ++s)
  {
    const float fS1 = (float)s;
    const float fS2 = (float)(s + 1);

    const float fCos1 = plMath::Cos(fS1 * stepAngle);
    const float fCos2 = plMath::Cos(fS2 * stepAngle);

    const float fSin1 = plMath::Sin(fS1 * stepAngle);
    const float fSin2 = plMath::Sin(fS2 * stepAngle);

    lines[curLine].m_start = transform * plVec3(fCos1 * fRadius, fSin1 * fRadius, fOffsetZ);
    lines[curLine].m_end = transform * plVec3(fCos2 * fRadius, fSin2 * fRadius, fOffsetZ);
    ++curLine;

    lines[curLine].m_start = transform * plVec3(fCos1 * fRadius, fSin1 * fRadius, -fOffsetZ);
    lines[curLine].m_end = transform * plVec3(fCos2 * fRadius, fSin2 * fRadius, -fOffsetZ);
    ++curLine;
  }

  // render top and bottom half circles
  for (plUInt32 s = 0; s < NUM_HALF_SEGMENTS; ++s)
  {
    const float fS1 = (float)s;
    const float fS2 = (float)(s + 1);

    const float fCos1 = plMath::Cos(fS1 * stepAngle);
    const float fCos2 = plMath::Cos(fS2 * stepAngle);

    const float fSin1 = plMath::Sin(fS1 * stepAngle);
    const float fSin2 = plMath::Sin(fS2 * stepAngle);

    // top two bows
    lines[curLine].m_start = transform * plVec3(0.0f, fCos1 * fRadius, fSin1 * fRadius + fOffsetZ);
    lines[curLine].m_end = transform * plVec3(0.0f, fCos2 * fRadius, fSin2 * fRadius + fOffsetZ);
    ++curLine;

    lines[curLine].m_start = transform * plVec3(fCos1 * fRadius, 0.0f, fSin1 * fRadius + fOffsetZ);
    lines[curLine].m_end = transform * plVec3(fCos2 * fRadius, 0.0f, fSin2 * fRadius + fOffsetZ);
    ++curLine;

    // bottom two bows
    lines[curLine].m_start = transform * plVec3(0.0f, fCos1 * fRadius, -fSin1 * fRadius - fOffsetZ);
    lines[curLine].m_end = transform * plVec3(0.0f, fCos2 * fRadius, -fSin2 * fRadius - fOffsetZ);
    ++curLine;

    lines[curLine].m_start = transform * plVec3(fCos1 * fRadius, 0.0f, -fSin1 * fRadius - fOffsetZ);
    lines[curLine].m_end = transform * plVec3(fCos2 * fRadius, 0.0f, -fSin2 * fRadius - fOffsetZ);
    ++curLine;
  }

  PLASMA_ASSERT_DEBUG(curLine == NUM_LINES, "Invalid line count");
  DrawLines(context, lines, color);
}

// static
void plDebugRenderer::DrawLineFrustum(const plDebugRendererContext& context, const plFrustum& frustum, const plColor& color, bool bDrawPlaneNormals /*= false*/)
{
  plVec3 cornerPoints[8];
  frustum.ComputeCornerPoints(cornerPoints);

  Line lines[12] = {
    Line(cornerPoints[plFrustum::FrustumCorner::NearBottomLeft], cornerPoints[plFrustum::FrustumCorner::FarBottomLeft]),
    Line(cornerPoints[plFrustum::FrustumCorner::NearBottomRight], cornerPoints[plFrustum::FrustumCorner::FarBottomRight]),
    Line(cornerPoints[plFrustum::FrustumCorner::NearTopLeft], cornerPoints[plFrustum::FrustumCorner::FarTopLeft]),
    Line(cornerPoints[plFrustum::FrustumCorner::NearTopRight], cornerPoints[plFrustum::FrustumCorner::FarTopRight]),

    Line(cornerPoints[plFrustum::FrustumCorner::NearBottomLeft], cornerPoints[plFrustum::FrustumCorner::NearBottomRight]),
    Line(cornerPoints[plFrustum::FrustumCorner::NearBottomRight], cornerPoints[plFrustum::FrustumCorner::NearTopRight]),
    Line(cornerPoints[plFrustum::FrustumCorner::NearTopRight], cornerPoints[plFrustum::FrustumCorner::NearTopLeft]),
    Line(cornerPoints[plFrustum::FrustumCorner::NearTopLeft], cornerPoints[plFrustum::FrustumCorner::NearBottomLeft]),

    Line(cornerPoints[plFrustum::FrustumCorner::FarBottomLeft], cornerPoints[plFrustum::FrustumCorner::FarBottomRight]),
    Line(cornerPoints[plFrustum::FrustumCorner::FarBottomRight], cornerPoints[plFrustum::FrustumCorner::FarTopRight]),
    Line(cornerPoints[plFrustum::FrustumCorner::FarTopRight], cornerPoints[plFrustum::FrustumCorner::FarTopLeft]),
    Line(cornerPoints[plFrustum::FrustumCorner::FarTopLeft], cornerPoints[plFrustum::FrustumCorner::FarBottomLeft]),
  };

  DrawLines(context, lines, color);

  if (bDrawPlaneNormals)
  {
    plColor normalColor = color + plColor(0.4f, 0.4f, 0.4f);
    float fDrawLength = 0.5f;

    const plVec3 nearPlaneNormal = frustum.GetPlane(0).m_vNormal * fDrawLength;
    const plVec3 farPlaneNormal = frustum.GetPlane(1).m_vNormal * fDrawLength;
    const plVec3 leftPlaneNormal = frustum.GetPlane(2).m_vNormal * fDrawLength;
    const plVec3 rightPlaneNormal = frustum.GetPlane(3).m_vNormal * fDrawLength;
    const plVec3 bottomPlaneNormal = frustum.GetPlane(4).m_vNormal * fDrawLength;
    const plVec3 topPlaneNormal = frustum.GetPlane(5).m_vNormal * fDrawLength;

    Line normalLines[24] = {
      Line(cornerPoints[plFrustum::FrustumCorner::NearBottomLeft], cornerPoints[plFrustum::FrustumCorner::NearBottomLeft] + nearPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::NearBottomRight], cornerPoints[plFrustum::FrustumCorner::NearBottomRight] + nearPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::NearTopLeft], cornerPoints[plFrustum::FrustumCorner::NearTopLeft] + nearPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::NearTopRight], cornerPoints[plFrustum::FrustumCorner::NearTopRight] + nearPlaneNormal),

      Line(cornerPoints[plFrustum::FrustumCorner::FarBottomLeft], cornerPoints[plFrustum::FrustumCorner::FarBottomLeft] + farPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::FarBottomRight], cornerPoints[plFrustum::FrustumCorner::FarBottomRight] + farPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::FarTopLeft], cornerPoints[plFrustum::FrustumCorner::FarTopLeft] + farPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::FarTopRight], cornerPoints[plFrustum::FrustumCorner::FarTopRight] + farPlaneNormal),

      Line(cornerPoints[plFrustum::FrustumCorner::NearBottomLeft], cornerPoints[plFrustum::FrustumCorner::NearBottomLeft] + leftPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::NearTopLeft], cornerPoints[plFrustum::FrustumCorner::NearTopLeft] + leftPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::FarBottomLeft], cornerPoints[plFrustum::FrustumCorner::FarBottomLeft] + leftPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::FarTopLeft], cornerPoints[plFrustum::FrustumCorner::FarTopLeft] + leftPlaneNormal),

      Line(cornerPoints[plFrustum::FrustumCorner::NearBottomRight], cornerPoints[plFrustum::FrustumCorner::NearBottomRight] + rightPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::NearTopRight], cornerPoints[plFrustum::FrustumCorner::NearTopRight] + rightPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::FarBottomRight], cornerPoints[plFrustum::FrustumCorner::FarBottomRight] + rightPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::FarTopRight], cornerPoints[plFrustum::FrustumCorner::FarTopRight] + rightPlaneNormal),

      Line(cornerPoints[plFrustum::FrustumCorner::NearBottomLeft], cornerPoints[plFrustum::FrustumCorner::NearBottomLeft] + bottomPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::NearBottomRight], cornerPoints[plFrustum::FrustumCorner::NearBottomRight] + bottomPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::FarBottomLeft], cornerPoints[plFrustum::FrustumCorner::FarBottomLeft] + bottomPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::FarBottomRight], cornerPoints[plFrustum::FrustumCorner::FarBottomRight] + bottomPlaneNormal),

      Line(cornerPoints[plFrustum::FrustumCorner::NearTopLeft], cornerPoints[plFrustum::FrustumCorner::NearTopLeft] + topPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::NearTopRight], cornerPoints[plFrustum::FrustumCorner::NearTopRight] + topPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::FarTopLeft], cornerPoints[plFrustum::FrustumCorner::FarTopLeft] + topPlaneNormal),
      Line(cornerPoints[plFrustum::FrustumCorner::FarTopRight], cornerPoints[plFrustum::FrustumCorner::FarTopRight] + topPlaneNormal),
    };

    DrawLines(context, normalLines, normalColor);
  }
}

// static
void plDebugRenderer::DrawSolidBox(const plDebugRendererContext& context, const plBoundingBox& box, const plColor& color, const plTransform& transform)
{
  PLASMA_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  auto& boxData = data.m_solidBoxes.ExpandAndGetRef();

  plTransform boxTransform(box.GetCenter(), plQuat::MakeIdentity(), box.GetHalfExtents());

  boxData.m_transform = transform * boxTransform;
  boxData.m_color = color;
}

// static
void plDebugRenderer::DrawSolidTriangles(const plDebugRendererContext& context, plArrayPtr<Triangle> triangles, const plColor& color)
{
  if (triangles.IsEmpty())
    return;

  PLASMA_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  for (auto& triangle : triangles)
  {
    const plColorLinearUB col = triangle.m_color * color;

    for (plUInt32 i = 0; i < 3; ++i)
    {
      auto& vertex = data.m_triangleVertices.ExpandAndGetRef();
      vertex.m_position = triangle.m_position[i];
      vertex.m_color = col;
    }
  }
}

void plDebugRenderer::DrawTexturedTriangles(const plDebugRendererContext& context, plArrayPtr<TexturedTriangle> triangles, const plColor& color, const plTexture2DResourceHandle& hTexture)
{
  if (triangles.IsEmpty())
    return;

  plResourceLock<plTexture2DResource> pTexture(hTexture, plResourceAcquireMode::AllowLoadingFallback);
  auto hResourceView = plGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture());

  PLASMA_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context).m_texTriangle3DVertices[hResourceView];

  for (auto& triangle : triangles)
  {
    const plColorLinearUB col = triangle.m_color * color;

    for (plUInt32 i = 0; i < 3; ++i)
    {
      auto& vertex = data.ExpandAndGetRef();
      vertex.m_position = triangle.m_position[i];
      vertex.m_texCoord = triangle.m_texcoord[i];
      vertex.m_color = col;
    }
  }
}

void plDebugRenderer::Draw2DRectangle(const plDebugRendererContext& context, const plRectFloat& rectInPixel, float fDepth, const plColor& color)
{
  Vertex vertices[6];

  vertices[0].m_position = plVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[1].m_position = plVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);
  vertices[2].m_position = plVec3(rectInPixel.Left(), rectInPixel.Bottom(), fDepth);
  vertices[3].m_position = plVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[4].m_position = plVec3(rectInPixel.Right(), rectInPixel.Top(), fDepth);
  vertices[5].m_position = plVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);

  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(vertices); ++i)
  {
    vertices[i].m_color = color;
  }


  PLASMA_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  data.m_triangle2DVertices.PushBackRange(plMakeArrayPtr(vertices));
}

void plDebugRenderer::Draw2DRectangle(const plDebugRendererContext& context, const plRectFloat& rectInPixel, float fDepth, const plColor& color, const plTexture2DResourceHandle& hTexture, plVec2 vScale)
{
  plResourceLock<plTexture2DResource> pTexture(hTexture, plResourceAcquireMode::AllowLoadingFallback);
  Draw2DRectangle(context, rectInPixel, fDepth, color, plGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()), vScale);
}

void plDebugRenderer::Draw2DRectangle(const plDebugRendererContext& context, const plRectFloat& rectInPixel, float fDepth, const plColor& color, plGALResourceViewHandle hResourceView, plVec2 vScale)
{
  TexVertex vertices[6];

  vertices[0].m_position = plVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[0].m_texCoord = plVec2(0, 0).CompMul(vScale);
  vertices[1].m_position = plVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);
  vertices[1].m_texCoord = plVec2(1, 1).CompMul(vScale);
  vertices[2].m_position = plVec3(rectInPixel.Left(), rectInPixel.Bottom(), fDepth);
  vertices[2].m_texCoord = plVec2(0, 1).CompMul(vScale);
  vertices[3].m_position = plVec3(rectInPixel.Left(), rectInPixel.Top(), fDepth);
  vertices[3].m_texCoord = plVec2(0, 0).CompMul(vScale);
  vertices[4].m_position = plVec3(rectInPixel.Right(), rectInPixel.Top(), fDepth);
  vertices[4].m_texCoord = plVec2(1, 0).CompMul(vScale);
  vertices[5].m_position = plVec3(rectInPixel.Right(), rectInPixel.Bottom(), fDepth);
  vertices[5].m_texCoord = plVec2(1, 1).CompMul(vScale);

  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(vertices); ++i)
  {
    vertices[i].m_color = color;
  }


  PLASMA_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  data.m_texTriangle2DVertices[hResourceView].PushBackRange(plMakeArrayPtr(vertices));
}

plUInt32 plDebugRenderer::Draw2DText(const plDebugRendererContext& context, const plFormatString& text, const plVec2I32& vPositionInPixel, const plColor& color, plUInt32 uiSizeInPixel /*= 16*/, plDebugTextHAlign::Enum horizontalAlignment /*= plDebugTextHAlign::Left*/, plDebugTextVAlign::Enum verticalAlignment /*= plDebugTextVAlign::Top*/)
{
  return AddTextLines(context, text, vPositionInPixel, (float)uiSizeInPixel, horizontalAlignment, verticalAlignment, [=](PerContextData& ref_data, plStringView sLine, plVec2 vTopLeftCorner) {
    auto& textLine = ref_data.m_textLines2D.ExpandAndGetRef();
    textLine.m_text = sLine;
    textLine.m_topLeftCorner = vTopLeftCorner;
    textLine.m_color = color;
    textLine.m_uiSizeInPixel = uiSizeInPixel; });
}


void plDebugRenderer::DrawInfoText(const plDebugRendererContext& context, plDebugTextPlacement::Enum placement, plStringView sGroupName, const plFormatString& text, const plColor& color)
{
  PLASMA_LOCK(s_Mutex);

  auto& data = GetDataForExtraction(context);

  plStringBuilder tmp;

  auto& e = data.m_infoTextData[(int)placement].ExpandAndGetRef();
  e.m_group = sGroupName;
  e.m_text = text.GetText(tmp);
  e.m_color = color;
}

plUInt32 plDebugRenderer::Draw3DText(const plDebugRendererContext& context, const plFormatString& text, const plVec3& vGlobalPosition, const plColor& color, plUInt32 uiSizeInPixel /*= 16*/, plDebugTextHAlign::Enum horizontalAlignment /*= plDebugTextHAlign::Center*/, plDebugTextVAlign::Enum verticalAlignment /*= plDebugTextVAlign::Bottom*/)
{
  return AddTextLines(context, text, plVec2I32(0), (float)uiSizeInPixel, horizontalAlignment, verticalAlignment, [&](PerContextData& ref_data, plStringView sLine, plVec2 vTopLeftCorner) {
    auto& textLine = ref_data.m_textLines3D.ExpandAndGetRef();
    textLine.m_text = sLine;
    textLine.m_topLeftCorner = vTopLeftCorner;
    textLine.m_color = color;
    textLine.m_uiSizeInPixel = uiSizeInPixel;
    textLine.m_position = vGlobalPosition; });
}

void plDebugRenderer::AddPersistentCross(const plDebugRendererContext& context, float fSize, const plColor& color, const plTransform& transform, plTime duration)
{
  PLASMA_LOCK(s_Mutex);

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_Crosses.ExpandAndGetRef();
  item.m_Transform = transform;
  item.m_Color = color;
  item.m_fSize = fSize;
  item.m_Timeout = data.m_Now + duration;
}

void plDebugRenderer::AddPersistentLineSphere(const plDebugRendererContext& context, float fRadius, const plColor& color, const plTransform& transform, plTime duration)
{
  PLASMA_LOCK(s_Mutex);

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_Spheres.ExpandAndGetRef();
  item.m_Transform = transform;
  item.m_Color = color;
  item.m_fRadius = fRadius;
  item.m_Timeout = data.m_Now + duration;
}

void plDebugRenderer::AddPersistentLineBox(const plDebugRendererContext& context, const plVec3& vHalfSize, const plColor& color, const plTransform& transform, plTime duration)
{
  PLASMA_LOCK(s_Mutex);

  auto& data = s_PersistentPerContextData[context];
  auto& item = data.m_Boxes.ExpandAndGetRef();
  item.m_Transform = transform;
  item.m_Color = color;
  item.m_vHalfSize = vHalfSize;
  item.m_Timeout = data.m_Now + duration;
}

void plDebugRenderer::DrawAngle(const plDebugRendererContext& context, plAngle startAngle, plAngle endAngle, const plColor& solidColor, const plColor& lineColor, const plTransform& transform, plVec3 vForwardAxis /*= plVec3::MakeAxisX()*/, plVec3 vRotationAxis /*= plVec3::MakeAxisZ()*/)
{
  plHybridArray<Triangle, 64> tris;
  plHybridArray<Line, 64> lines;

  startAngle.NormalizeRange();
  endAngle.NormalizeRange();

  if (startAngle > endAngle)
    startAngle -= plAngle::MakeFromDegree(360);

  const plAngle range = endAngle - startAngle;
  const plUInt32 uiTesselation = plMath::Max(1u, (plUInt32)(range / plAngle::MakeFromDegree(5)));
  const plAngle step = range / (float)uiTesselation;

  plQuat qStart = plQuat::MakeFromAxisAndAngle(vRotationAxis, startAngle);

  plQuat qStep = plQuat::MakeFromAxisAndAngle(vRotationAxis, step);

  plVec3 vCurDir = qStart * vForwardAxis;

  if (lineColor.a > 0)
  {
    Line& l1 = lines.ExpandAndGetRef();
    l1.m_start.SetZero();
    l1.m_end = vCurDir;
  }

  for (plUInt32 i = 0; i < uiTesselation; ++i)
  {
    const plVec3 vNextDir = qStep * vCurDir;

    if (solidColor.a > 0)
    {
      Triangle& tri1 = tris.ExpandAndGetRef();
      tri1.m_position[0] = transform.m_vPosition;
      tri1.m_position[1] = transform.TransformPosition(vNextDir);
      tri1.m_position[2] = transform.TransformPosition(vCurDir);

      Triangle& tri2 = tris.ExpandAndGetRef();
      tri2.m_position[0] = transform.m_vPosition;
      tri2.m_position[1] = transform.TransformPosition(vCurDir);
      tri2.m_position[2] = transform.TransformPosition(vNextDir);
    }

    if (lineColor.a > 0)
    {
      Line& l1 = lines.ExpandAndGetRef();
      l1.m_start.SetZero();
      l1.m_end = vNextDir;

      Line& l2 = lines.ExpandAndGetRef();
      l2.m_start = vCurDir;
      l2.m_end = vNextDir;
    }

    vCurDir = vNextDir;
  }

  DrawSolidTriangles(context, tris, solidColor);
  DrawLines(context, lines, lineColor, transform);
}

void plDebugRenderer::DrawOpeningCone(const plDebugRendererContext& context, plAngle halfAngle, const plColor& colorInside, const plColor& colorOutside, const plTransform& transform, plVec3 vForwardAxis /*= plVec3::MakeAxisX()*/)
{
  plHybridArray<Triangle, 64> trisInside;
  plHybridArray<Triangle, 64> trisOutside;

  halfAngle = plMath::Clamp(halfAngle, plAngle(), plAngle::MakeFromDegree(180));

  const plAngle refAngle = halfAngle <= plAngle::MakeFromDegree(90) ? halfAngle : plAngle::MakeFromDegree(180) - halfAngle;
  const plUInt32 uiTesselation = plMath::Max(8u, (plUInt32)(refAngle / plAngle::MakeFromDegree(2)));

  const plVec3 tangentAxis = vForwardAxis.GetOrthogonalVector().GetNormalized();

  plQuat tilt = plQuat::MakeFromAxisAndAngle(tangentAxis, halfAngle);

  plQuat step = plQuat::MakeFromAxisAndAngle(vForwardAxis, plAngle::MakeFromDegree(360) / (float)uiTesselation);

  plVec3 vCurDir = tilt * vForwardAxis;

  for (plUInt32 i = 0; i < uiTesselation; ++i)
  {
    const plVec3 vNextDir = step * vCurDir;

    if (colorInside.a > 0)
    {
      Triangle& tri = trisInside.ExpandAndGetRef();
      tri.m_position[0] = transform.m_vPosition;
      tri.m_position[1] = transform.TransformPosition(vCurDir);
      tri.m_position[2] = transform.TransformPosition(vNextDir);
    }

    if (colorOutside.a > 0)
    {
      Triangle& tri = trisOutside.ExpandAndGetRef();
      tri.m_position[0] = transform.m_vPosition;
      tri.m_position[1] = transform.TransformPosition(vNextDir);
      tri.m_position[2] = transform.TransformPosition(vCurDir);
    }

    vCurDir = vNextDir;
  }


  DrawSolidTriangles(context, trisInside, colorInside);
  DrawSolidTriangles(context, trisOutside, colorOutside);
}

void plDebugRenderer::DrawLimitCone(const plDebugRendererContext& context, plAngle halfAngle1, plAngle halfAngle2, const plColor& solidColor, const plColor& lineColor, const plTransform& transform)
{
  constexpr plUInt32 NUM_LINES = 32;
  plHybridArray<Line, NUM_LINES * 2> lines;
  plHybridArray<Triangle, NUM_LINES * 2> tris;

  // no clue how this works
  // copied 1:1 from NVIDIA's PhysX SDK: Cm::visualizeLimitCone
  {
    float scale = 1.0f;

    const float tanQSwingZ = plMath::Tan(halfAngle1 / 4);
    const float tanQSwingY = plMath::Tan(halfAngle2 / 4);

    plVec3 prev(0);
    for (plUInt32 i = 0; i <= NUM_LINES; i++)
    {
      const float angle = 2 * plMath::Pi<float>() / NUM_LINES * i;
      const float c = plMath::Cos(plAngle::MakeFromRadian(angle)), s = plMath::Sin(plAngle::MakeFromRadian(angle));
      const plVec3 rv(0, -tanQSwingZ * s, tanQSwingY * c);
      const float rv2 = rv.GetLengthSquared();
      const float r = (1 / (1 + rv2));
      const plQuat q = plQuat(0, r * 2 * rv.y, r * 2 * rv.z, r * (1 - rv2));
      const plVec3 a = q * plVec3(1.0f, 0, 0) * scale;

      if (lineColor.a > 0)
      {
        auto& l1 = lines.ExpandAndGetRef();
        l1.m_start = prev;
        l1.m_end = a;

        auto& l2 = lines.ExpandAndGetRef();
        l2.m_start.SetZero();
        l2.m_end = a;
      }

      if (solidColor.a > 0)
      {
        auto& t1 = tris.ExpandAndGetRef();
        t1.m_position[0] = transform.m_vPosition;
        t1.m_position[1] = transform.TransformPosition(prev);
        t1.m_position[2] = transform.TransformPosition(a);

        auto& t2 = tris.ExpandAndGetRef();
        t2.m_position[0] = transform.m_vPosition;
        t2.m_position[1] = transform.TransformPosition(a);
        t2.m_position[2] = transform.TransformPosition(prev);
      }

      prev = a;
    }
  }

  DrawSolidTriangles(context, tris, solidColor);
  DrawLines(context, lines, lineColor, transform);
}

void plDebugRenderer::DrawCylinder(const plDebugRendererContext& context, float fRadiusStart, float fRadiusEnd, float fLength, const plColor& solidColor, const plColor& lineColor, const plTransform& transform, bool bCapStart /*= false*/, bool bCapEnd /*= false*/)
{
  constexpr plUInt32 NUM_SEGMENTS = 16;
  plHybridArray<Line, NUM_SEGMENTS * 3> lines;
  plHybridArray<Triangle, NUM_SEGMENTS * 2 * 2> tris;

  const plAngle step = plAngle::MakeFromDegree(360) / NUM_SEGMENTS;
  plAngle angle = {};

  plVec3 vCurCircle(0, 1 /*plMath::Cos(angle)*/, 0 /*plMath::Sin(angle)*/);

  const bool bSolid = solidColor.a > 0;
  const bool bLine = lineColor.a > 0;

  const plVec3 vLastCircle(0, plMath::Cos(-step), plMath::Sin(-step));
  const plVec3 vLastStart = transform.TransformPosition(plVec3(0, vLastCircle.y * fRadiusStart, vLastCircle.z * fRadiusStart));
  const plVec3 vLastEnd = transform.TransformPosition(plVec3(fLength, vLastCircle.y * fRadiusEnd, vLastCircle.z * fRadiusEnd));

  for (plUInt32 i = 0; i < NUM_SEGMENTS; ++i)
  {
    angle += step;
    const plVec3 vNextCircle(0, plMath::Cos(angle), plMath::Sin(angle));

    plVec3 vCurStart = vCurCircle * fRadiusStart;
    plVec3 vNextStart = vNextCircle * fRadiusStart;

    plVec3 vCurEnd(fLength, vCurCircle.y * fRadiusEnd, vCurCircle.z * fRadiusEnd);
    plVec3 vNextEnd(fLength, vNextCircle.y * fRadiusEnd, vNextCircle.z * fRadiusEnd);

    if (bLine)
    {
      lines.PushBack({vCurStart, vNextStart});
      lines.PushBack({vCurEnd, vNextEnd});
      lines.PushBack({vCurStart, vCurEnd});
    }

    if (bSolid)
    {
      vCurStart = transform.TransformPosition(vCurStart);
      vCurEnd = transform.TransformPosition(vCurEnd);
      vNextStart = transform.TransformPosition(vNextStart);
      vNextEnd = transform.TransformPosition(vNextEnd);

      tris.PushBack({vCurStart, vNextStart, vNextEnd});
      tris.PushBack({vCurStart, vNextEnd, vCurEnd});

      if (bCapStart)
        tris.PushBack({vLastStart, vNextStart, vCurStart});

      if (bCapEnd)
        tris.PushBack({vLastEnd, vCurEnd, vNextEnd});
    }

    vCurCircle = vNextCircle;
  }

  DrawSolidTriangles(context, tris, solidColor);
  DrawLines(context, lines, lineColor, transform);
}

// static
void plDebugRenderer::Render(const plRenderViewContext& renderViewContext)
{
  if (renderViewContext.m_pWorldDebugContext != nullptr)
  {
    RenderInternal(*renderViewContext.m_pWorldDebugContext, renderViewContext);
  }

  if (renderViewContext.m_pViewDebugContext != nullptr)
  {
    RenderInternal(*renderViewContext.m_pViewDebugContext, renderViewContext);
  }
}

// static
void plDebugRenderer::RenderInternal(const plDebugRendererContext& context, const plRenderViewContext& renderViewContext)
{
  {
    PLASMA_LOCK(s_Mutex);

    auto& data = s_PersistentPerContextData[context];
    data.m_Now = plTime::Now();

    // persistent crosses
    {
      plUInt32 uiNumItems = data.m_Crosses.GetCount();
      for (plUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Crosses[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_Crosses.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          plDebugRenderer::DrawCross(context, plVec3::MakeZero(), item.m_fSize, item.m_Color, item.m_Transform);

          ++i;
        }
      }
    }

    // persistent spheres
    {
      plUInt32 uiNumItems = data.m_Spheres.GetCount();
      for (plUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Spheres[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_Spheres.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          plDebugRenderer::DrawLineSphere(context, plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), item.m_fRadius), item.m_Color, item.m_Transform);

          ++i;
        }
      }
    }

    // persistent boxes
    {
      plUInt32 uiNumItems = data.m_Boxes.GetCount();
      for (plUInt32 i = 0; i < uiNumItems;)
      {
        const auto& item = data.m_Boxes[i];

        if (data.m_Now > item.m_Timeout)
        {
          data.m_Boxes.RemoveAtAndSwap(i);
          --uiNumItems;
        }
        else
        {
          plDebugRenderer::DrawLineBox(context, plBoundingBox::MakeFromMinMax(-item.m_vHalfSize, item.m_vHalfSize), item.m_Color, item.m_Transform);

          ++i;
        }
      }
    }
  }

  DoubleBufferedPerContextData* pDoubleBufferedContextData = nullptr;
  if (!s_PerContextData.TryGetValue(context, pDoubleBufferedContextData))
  {
    return;
  }

  PerContextData* pData = pDoubleBufferedContextData->m_pData[plRenderWorld::GetDataIndexForRendering()].Borrow();
  if (pData == nullptr)
  {
    return;
  }

  // draw info text
  {
    static_assert((int)plDebugTextPlacement::ENUM_COUNT == 6);

    plDebugTextHAlign::Enum ha[(int)plDebugTextPlacement::ENUM_COUNT] = {
      plDebugTextHAlign::Left,
      plDebugTextHAlign::Center,
      plDebugTextHAlign::Right,
      plDebugTextHAlign::Left,
      plDebugTextHAlign::Center,
      plDebugTextHAlign::Right};

    plDebugTextVAlign::Enum va[(int)plDebugTextPlacement::ENUM_COUNT] = {
      plDebugTextVAlign::Top,
      plDebugTextVAlign::Top,
      plDebugTextVAlign::Top,
      plDebugTextVAlign::Bottom,
      plDebugTextVAlign::Bottom,
      plDebugTextVAlign::Bottom};

    int offs[(int)plDebugTextPlacement::ENUM_COUNT] = {20, 20, 20, -20, -20, -20};

    plInt32 resX = (plInt32)renderViewContext.m_pViewData->m_ViewPortRect.width;
    plInt32 resY = (plInt32)renderViewContext.m_pViewData->m_ViewPortRect.height;

    plVec2I32 anchor[(int)plDebugTextPlacement::ENUM_COUNT] = {
      plVec2I32(10, 10),
      plVec2I32(resX / 2, 10),
      plVec2I32(resX - 10, 10),
      plVec2I32(10, resY - 10),
      plVec2I32(resX / 2, resY - 10),
      plVec2I32(resX - 10, resY - 10)};

    for (plUInt32 corner = 0; corner < (plUInt32)plDebugTextPlacement::ENUM_COUNT; ++corner)
    {
      auto& cd = pData->m_infoTextData[corner];

      // InsertionSort is stable
      plSorting::InsertionSort(cd, [](const InfoTextData& lhs, const InfoTextData& rhs) -> bool { return lhs.m_group < rhs.m_group; });

      plVec2I32 pos = anchor[corner];

      for (plUInt32 i = 0; i < cd.GetCount(); ++i)
      {
        // add some space between groups
        if (i > 0 && cd[i - 1].m_group != cd[i].m_group)
          pos.y += offs[corner];

        pos.y += offs[corner] * Draw2DText(context, cd[i].m_text.GetData(), pos, cd[i].m_color, 16, ha[corner], va[corner]);
      }
    }
  }

  // update the frame counter
  pDoubleBufferedContextData->m_uiLastRenderedFrame = plRenderWorld::GetFrameCounter();

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plGALCommandEncoder* pGALCommandEncoder = renderViewContext.m_pRenderContext->GetCommandEncoder();

  // SolidBoxes
  {
    plUInt32 uiNumSolidBoxes = pData->m_solidBoxes.GetCount();
    if (uiNumSolidBoxes != 0)
    {
      CreateDataBuffer(BufferType::SolidBoxes, sizeof(BoxData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugGeometryShader);
      renderViewContext.m_pRenderContext->BindBuffer("boxData", pDevice->GetDefaultResourceView(s_hDataBuffer[BufferType::SolidBoxes]));
      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hSolidBoxMeshBuffer);

      const BoxData* pSolidBoxData = pData->m_solidBoxes.GetData();
      while (uiNumSolidBoxes > 0)
      {
        const plUInt32 uiNumSolidBoxesInBatch = plMath::Min<plUInt32>(uiNumSolidBoxes, BOXES_PER_BATCH);
        pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::SolidBoxes], 0, plMakeArrayPtr(pSolidBoxData, uiNumSolidBoxesInBatch).ToByteArray());

        unsigned int uiRenderedInstances = uiNumSolidBoxesInBatch;
        if (renderViewContext.m_pCamera->IsStereoscopic())
          uiRenderedInstances *= 2;

        renderViewContext.m_pRenderContext->DrawMeshBuffer(0xFFFFFFFF, 0, uiRenderedInstances).IgnoreResult();

        uiNumSolidBoxes -= uiNumSolidBoxesInBatch;
        pSolidBoxData += BOXES_PER_BATCH;
      }
    }
  }

  // Triangles
  {
    plUInt32 uiNumTriangleVertices = pData->m_triangleVertices.GetCount();
    if (uiNumTriangleVertices != 0)
    {
      CreateVertexBuffer(BufferType::Triangles3D, sizeof(Vertex));

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pTriangleData = pData->m_triangleVertices.GetData();
      while (uiNumTriangleVertices > 0)
      {
        const plUInt32 uiNumTriangleVerticesInBatch = plMath::Min<plUInt32>(uiNumTriangleVertices, TRIANGLE_VERTICES_PER_BATCH);
        PLASMA_ASSERT_DEV(uiNumTriangleVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
        pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::Triangles3D], 0, plMakeArrayPtr(pTriangleData, uiNumTriangleVerticesInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::Triangles3D], plGALBufferHandle(), &s_VertexDeclarationInfo, plGALPrimitiveTopology::Triangles, uiNumTriangleVerticesInBatch / 3);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumTriangleVertices -= uiNumTriangleVerticesInBatch;
        pTriangleData += TRIANGLE_VERTICES_PER_BATCH;
      }
    }
  }

  // Textured 3D triangles
  {
    for (auto itTex = pData->m_texTriangle3DVertices.GetIterator(); itTex.IsValid(); ++itTex)
    {
      renderViewContext.m_pRenderContext->BindTexture2D("BaseTexture", itTex.Key());

      const auto& verts = itTex.Value();

      plUInt32 uiNumVertices = verts.GetCount();
      if (uiNumVertices != 0)
      {
        CreateVertexBuffer(BufferType::TexTriangles3D, sizeof(TexVertex));

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
        renderViewContext.m_pRenderContext->BindShader(s_hDebugTexturedPrimitiveShader);

        const TexVertex* pTriangleData = verts.GetData();
        while (uiNumVertices > 0)
        {
          const plUInt32 uiNumVerticesInBatch = plMath::Min<plUInt32>(uiNumVertices, TEX_TRIANGLE_VERTICES_PER_BATCH);
          PLASMA_ASSERT_DEV(uiNumVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
          pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::TexTriangles3D], 0, plMakeArrayPtr(pTriangleData, uiNumVerticesInBatch).ToByteArray());

          renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::TexTriangles3D], plGALBufferHandle(), &s_TexVertexDeclarationInfo, plGALPrimitiveTopology::Triangles, uiNumVerticesInBatch / 3);

          renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

          uiNumVertices -= uiNumVerticesInBatch;
          pTriangleData += TEX_TRIANGLE_VERTICES_PER_BATCH;
        }
      }
    }
  }

  // 3D Lines
  {
    plUInt32 uiNumLineVertices = pData->m_lineVertices.GetCount();
    if (uiNumLineVertices != 0)
    {
      CreateVertexBuffer(BufferType::Lines, sizeof(Vertex));

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pLineData = pData->m_lineVertices.GetData();
      while (uiNumLineVertices > 0)
      {
        const plUInt32 uiNumLineVerticesInBatch = plMath::Min<plUInt32>(uiNumLineVertices, LINE_VERTICES_PER_BATCH);
        PLASMA_ASSERT_DEV(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");
        pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::Lines], 0, plMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::Lines], plGALBufferHandle(), &s_VertexDeclarationInfo, plGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumLineVertices -= uiNumLineVerticesInBatch;
        pLineData += LINE_VERTICES_PER_BATCH;
      }
    }
  }

  // 2D Lines
  {
    plUInt32 uiNumLineVertices = pData->m_line2DVertices.GetCount();
    if (uiNumLineVertices != 0)
    {
      CreateVertexBuffer(BufferType::Lines2D, sizeof(Vertex));

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "TRUE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pLineData = pData->m_line2DVertices.GetData();
      while (uiNumLineVertices > 0)
      {
        const plUInt32 uiNumLineVerticesInBatch = plMath::Min<plUInt32>(uiNumLineVertices, LINE_VERTICES_PER_BATCH);
        PLASMA_ASSERT_DEV(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");
        pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::Lines2D], 0, plMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::Lines2D], plGALBufferHandle(), &s_VertexDeclarationInfo, plGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumLineVertices -= uiNumLineVerticesInBatch;
        pLineData += LINE_VERTICES_PER_BATCH;
      }
    }
  }

  // LineBoxes
  {
    plUInt32 uiNumLineBoxes = pData->m_lineBoxes.GetCount();
    if (uiNumLineBoxes != 0)
    {
      CreateDataBuffer(BufferType::LineBoxes, sizeof(BoxData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugGeometryShader);
      renderViewContext.m_pRenderContext->BindBuffer("boxData", pDevice->GetDefaultResourceView(s_hDataBuffer[BufferType::LineBoxes]));
      renderViewContext.m_pRenderContext->BindMeshBuffer(s_hLineBoxMeshBuffer);

      const BoxData* pLineBoxData = pData->m_lineBoxes.GetData();
      while (uiNumLineBoxes > 0)
      {
        const plUInt32 uiNumLineBoxesInBatch = plMath::Min<plUInt32>(uiNumLineBoxes, BOXES_PER_BATCH);
        pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::LineBoxes], 0, plMakeArrayPtr(pLineBoxData, uiNumLineBoxesInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->DrawMeshBuffer(0xFFFFFFFF, 0, uiNumLineBoxesInBatch).IgnoreResult();

        uiNumLineBoxes -= uiNumLineBoxesInBatch;
        pLineBoxData += BOXES_PER_BATCH;
      }
    }
  }

  // 2D Rectangles
  {
    plUInt32 uiNum2DVertices = pData->m_triangle2DVertices.GetCount();
    if (uiNum2DVertices != 0)
    {
      CreateVertexBuffer(BufferType::Triangles2D, sizeof(Vertex));

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "TRUE");
      renderViewContext.m_pRenderContext->BindShader(s_hDebugPrimitiveShader);

      const Vertex* pTriangleData = pData->m_triangle2DVertices.GetData();
      while (uiNum2DVertices > 0)
      {
        const plUInt32 uiNum2DVerticesInBatch = plMath::Min<plUInt32>(uiNum2DVertices, TRIANGLE_VERTICES_PER_BATCH);
        PLASMA_ASSERT_DEV(uiNum2DVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
        pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::Triangles2D], 0, plMakeArrayPtr(pTriangleData, uiNum2DVerticesInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::Triangles2D], plGALBufferHandle(), &s_VertexDeclarationInfo, plGALPrimitiveTopology::Triangles, uiNum2DVerticesInBatch / 3);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNum2DVertices -= uiNum2DVerticesInBatch;
        pTriangleData += TRIANGLE_VERTICES_PER_BATCH;
      }
    }
  }

  // Textured 2D triangles
  {
    for (auto itTex = pData->m_texTriangle2DVertices.GetIterator(); itTex.IsValid(); ++itTex)
    {
      renderViewContext.m_pRenderContext->BindTexture2D("BaseTexture", itTex.Key());

      const auto& verts = itTex.Value();

      plUInt32 uiNum2DVertices = verts.GetCount();
      if (uiNum2DVertices != 0)
      {
        CreateVertexBuffer(BufferType::TexTriangles2D, sizeof(TexVertex));

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "TRUE");
        renderViewContext.m_pRenderContext->BindShader(s_hDebugTexturedPrimitiveShader);

        const TexVertex* pTriangleData = verts.GetData();
        while (uiNum2DVertices > 0)
        {
          const plUInt32 uiNum2DVerticesInBatch = plMath::Min<plUInt32>(uiNum2DVertices, TEX_TRIANGLE_VERTICES_PER_BATCH);
          PLASMA_ASSERT_DEV(uiNum2DVerticesInBatch % 3 == 0, "Vertex count must be a multiple of 3.");
          pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::TexTriangles2D], 0, plMakeArrayPtr(pTriangleData, uiNum2DVerticesInBatch).ToByteArray());

          renderViewContext.m_pRenderContext->BindMeshBuffer(s_hDataBuffer[BufferType::TexTriangles2D], plGALBufferHandle(), &s_TexVertexDeclarationInfo, plGALPrimitiveTopology::Triangles, uiNum2DVerticesInBatch / 3);

          renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

          uiNum2DVertices -= uiNum2DVerticesInBatch;
          pTriangleData += TEX_TRIANGLE_VERTICES_PER_BATCH;
        }
      }
    }
  }

  // Text
  {
    pData->m_glyphs.Clear();

    for (auto& textLine : pData->m_textLines3D)
    {
      plVec3 screenPos;
      if (renderViewContext.m_pViewData->ComputeScreenSpacePos(textLine.m_position, screenPos).Succeeded() && screenPos.z > 0.0f)
      {
        textLine.m_topLeftCorner.x += plMath::Round(screenPos.x);
        textLine.m_topLeftCorner.y += plMath::Round(screenPos.y);

        AppendGlyphs(pData->m_glyphs, textLine);
      }
    }

    for (auto& textLine : pData->m_textLines2D)
    {
      AppendGlyphs(pData->m_glyphs, textLine);
    }


    plUInt32 uiNumGlyphs = pData->m_glyphs.GetCount();
    if (uiNumGlyphs != 0)
    {
      CreateDataBuffer(BufferType::Glyphs, sizeof(GlyphData));

      renderViewContext.m_pRenderContext->BindShader(s_hDebugTextShader);
      renderViewContext.m_pRenderContext->BindBuffer("glyphData", pDevice->GetDefaultResourceView(s_hDataBuffer[BufferType::Glyphs]));
      renderViewContext.m_pRenderContext->BindTexture2D("FontTexture", s_hDebugFontTexture);

      const GlyphData* pGlyphData = pData->m_glyphs.GetData();
      while (uiNumGlyphs > 0)
      {
        const plUInt32 uiNumGlyphsInBatch = plMath::Min<plUInt32>(uiNumGlyphs, GLYPHS_PER_BATCH);
        pGALCommandEncoder->UpdateBuffer(s_hDataBuffer[BufferType::Glyphs], 0, plMakeArrayPtr(pGlyphData, uiNumGlyphsInBatch).ToByteArray());

        renderViewContext.m_pRenderContext->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, uiNumGlyphsInBatch * 2);

        renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

        uiNumGlyphs -= uiNumGlyphsInBatch;
        pGlyphData += GLYPHS_PER_BATCH;
      }
    }
  }
}

void plDebugRenderer::OnEngineStartup()
{
  {
    plGeometry geom;
    geom.AddLineBox(plVec3(2.0f));

    plMeshBufferResourceDescriptor desc;
    desc.AddStream(plGALVertexAttributeSemantic::Position, plGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, plGALPrimitiveTopology::Lines);

    s_hLineBoxMeshBuffer = plResourceManager::CreateResource<plMeshBufferResource>("DebugLineBox", std::move(desc), "Mesh for Rendering Debug Line Boxes");
  }

  {
    plGeometry geom;
    geom.AddBox(plVec3(2.0f), false);

    plMeshBufferResourceDescriptor desc;
    desc.AddStream(plGALVertexAttributeSemantic::Position, plGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, plGALPrimitiveTopology::Triangles);

    s_hSolidBoxMeshBuffer = plResourceManager::CreateResource<plMeshBufferResource>("DebugSolidBox", std::move(desc), "Mesh for Rendering Debug Solid Boxes");
  }

  {
    // reset, if already used before
    s_VertexDeclarationInfo.m_VertexStreams.Clear();

    {
      plVertexStreamInfo& si = s_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = plGALVertexAttributeSemantic::Position;
      si.m_Format = plGALResourceFormat::XYZFloat;
      si.m_uiOffset = 0;
      si.m_uiElementSize = 12;
    }

    {
      plVertexStreamInfo& si = s_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = plGALVertexAttributeSemantic::Color0;
      si.m_Format = plGALResourceFormat::RGBAUByteNormalized;
      si.m_uiOffset = 12;
      si.m_uiElementSize = 4;
    }
  }

  {
    // reset, if already used before
    s_TexVertexDeclarationInfo.m_VertexStreams.Clear();

    {
      plVertexStreamInfo& si = s_TexVertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = plGALVertexAttributeSemantic::Position;
      si.m_Format = plGALResourceFormat::XYZFloat;
      si.m_uiOffset = 0;
      si.m_uiElementSize = 12;
    }

    {
      plVertexStreamInfo& si = s_TexVertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = plGALVertexAttributeSemantic::Color0;
      si.m_Format = plGALResourceFormat::RGBAUByteNormalized;
      si.m_uiOffset = 12;
      si.m_uiElementSize = 4;
    }

    {
      plVertexStreamInfo& si = s_TexVertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = plGALVertexAttributeSemantic::TexCoord0;
      si.m_Format = plGALResourceFormat::XYFloat;
      si.m_uiOffset = 16;
      si.m_uiElementSize = 8;
    }

    {
      plVertexStreamInfo& si = s_TexVertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = plGALVertexAttributeSemantic::TexCoord1; // padding
      si.m_Format = plGALResourceFormat::XYFloat;
      si.m_uiOffset = 24;
      si.m_uiElementSize = 8;
    }
  }

  {
    plImage debugFontImage;
    plGraphicsUtils::CreateSimpleASCIIFontTexture(debugFontImage);

    plGALSystemMemoryDescription memoryDesc;
    memoryDesc.m_pData = debugFontImage.GetPixelPointer<plUInt8>();
    memoryDesc.m_uiRowPitch = static_cast<plUInt32>(debugFontImage.GetRowPitch());
    memoryDesc.m_uiSlicePitch = static_cast<plUInt32>(debugFontImage.GetDepthPitch());

    plTexture2DResourceDescriptor desc;
    desc.m_DescGAL.m_uiWidth = debugFontImage.GetWidth();
    desc.m_DescGAL.m_uiHeight = debugFontImage.GetHeight();
    desc.m_DescGAL.m_Format = plGALResourceFormat::RGBAUByteNormalized;
    desc.m_InitialContent = plMakeArrayPtr(&memoryDesc, 1);

    s_hDebugFontTexture = plResourceManager::CreateResource<plTexture2DResource>("DebugFontTexture", std::move(desc));
  }

  s_hDebugGeometryShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Debug/DebugGeometry.plShader");
  s_hDebugPrimitiveShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Debug/DebugPrimitive.plShader");
  s_hDebugTexturedPrimitiveShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Debug/DebugTexturedPrimitive.plShader");
  s_hDebugTextShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Debug/DebugText.plShader");

  plRenderWorld::GetRenderEvent().AddEventHandler(&OnRenderEvent);
}

void plDebugRenderer::OnEngineShutdown()
{
  plRenderWorld::GetRenderEvent().RemoveEventHandler(&OnRenderEvent);

  for (plUInt32 i = 0; i < BufferType::Count; ++i)
  {
    DestroyBuffer(static_cast<BufferType::Enum>(i));
  }

  s_hLineBoxMeshBuffer.Invalidate();
  s_hSolidBoxMeshBuffer.Invalidate();
  s_hDebugFontTexture.Invalidate();

  s_hDebugGeometryShader.Invalidate();
  s_hDebugPrimitiveShader.Invalidate();
  s_hDebugTexturedPrimitiveShader.Invalidate();
  s_hDebugTextShader.Invalidate();

  s_PerContextData.Clear();

  s_PersistentPerContextData.Clear();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plScriptExtensionClass_Debug, plNoBase, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(DrawCross, In, "World", In, "Position", In, "Size", In, "Color", In, "Transform")->AddAttributes(
      new plFunctionArgumentAttributes(2, new plDefaultValueAttribute(0.1f)),
      new plFunctionArgumentAttributes(3, new plExposeColorAlphaAttribute())),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(DrawLineBox, In, "World", In, "Position", In, "HalfExtents", In, "Color", In, "Transform")->AddAttributes(
      new plFunctionArgumentAttributes(2, new plDefaultValueAttribute(plVec3(1))),
      new plFunctionArgumentAttributes(3, new plExposeColorAlphaAttribute())),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(DrawLineSphere, In, "World", In, "Position", In, "Radius", In, "Color", In, "Transform")->AddAttributes(
      new plFunctionArgumentAttributes(2, new plDefaultValueAttribute(1.0f)),
      new plFunctionArgumentAttributes(3, new plExposeColorAlphaAttribute())),

    PLASMA_SCRIPT_FUNCTION_PROPERTY(DrawSolidBox, In, "World", In, "Position", In, "HalfExtents", In, "Color", In, "Transform")->AddAttributes(
      new plFunctionArgumentAttributes(2, new plDefaultValueAttribute(plVec3(1))),
      new plFunctionArgumentAttributes(3, new plExposeColorAlphaAttribute())),
    
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Draw2DText, In, "World", In, "Text", In, "Position", In, "Color", In, "SizeInPixel", In, "HAlign")->AddAttributes(
      new plFunctionArgumentAttributes(4, new plDefaultValueAttribute(16))),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Draw3DText, In, "World", In, "Text", In, "Position", In, "Color", In, "SizeInPixel")->AddAttributes(
      new plFunctionArgumentAttributes(4, new plDefaultValueAttribute(16))),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(DrawInfoText, In, "World", In, "Text", In, "Placement", In, "Group", In, "Color"),

    PLASMA_SCRIPT_FUNCTION_PROPERTY(AddPersistentCross, In, "World", In, "Position", In, "Size", In, "Color", In, "Transform", In, "Duration")->AddAttributes(
      new plFunctionArgumentAttributes(2, new plDefaultValueAttribute(0.1f)),
      new plFunctionArgumentAttributes(3, new plExposeColorAlphaAttribute()),
      new plFunctionArgumentAttributes(5, new plDefaultValueAttribute(plTime::MakeFromSeconds(1)))),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(AddPersistentLineBox, In, "World", In, "Position", In, "HalfExtents", In, "Color", In, "Transform", In, "Duration")->AddAttributes(
      new plFunctionArgumentAttributes(2, new plDefaultValueAttribute(plVec3(1))),
      new plFunctionArgumentAttributes(3, new plExposeColorAlphaAttribute()),
      new plFunctionArgumentAttributes(5, new plDefaultValueAttribute(plTime::MakeFromSeconds(1)))),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(AddPersistentLineSphere, In, "World", In, "Position", In, "Radius", In, "Color", In, "Transform", In, "Duration")->AddAttributes(
      new plFunctionArgumentAttributes(2, new plDefaultValueAttribute(1.0f)),
      new plFunctionArgumentAttributes(3, new plExposeColorAlphaAttribute()),
      new plFunctionArgumentAttributes(5, new plDefaultValueAttribute(plTime::MakeFromSeconds(1)))),
  }
  PLASMA_END_FUNCTIONS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plScriptExtensionAttribute("Debug"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

// static
void plScriptExtensionClass_Debug::DrawCross(const plWorld* pWorld, const plVec3& vPosition, float fSize, const plColor& color, const plTransform& transform)
{
  plDebugRenderer::DrawCross(pWorld, vPosition, fSize, color, transform);
}

// static
void plScriptExtensionClass_Debug::DrawLineBox(const plWorld* pWorld, const plVec3& vPosition, const plVec3& vHalfExtents, const plColor& color, const plTransform& transform)
{
  plDebugRenderer::DrawLineBox(pWorld, plBoundingBox::MakeFromCenterAndHalfExtents(vPosition, vHalfExtents), color, transform);
}

// static
void plScriptExtensionClass_Debug::DrawLineSphere(const plWorld* pWorld, const plVec3& vPosition, float fRadius, const plColor& color, const plTransform& transform)
{
  plDebugRenderer::DrawLineSphere(pWorld, plBoundingSphere::MakeFromCenterAndRadius(vPosition, fRadius), color, transform);
}

// static
void plScriptExtensionClass_Debug::DrawSolidBox(const plWorld* pWorld, const plVec3& vPosition, const plVec3& vHalfExtents, const plColor& color, const plTransform& transform)
{
  plDebugRenderer::DrawSolidBox(pWorld, plBoundingBox::MakeFromCenterAndHalfExtents(vPosition, vHalfExtents), color, transform);
}

// static
void plScriptExtensionClass_Debug::Draw2DText(const plWorld* pWorld, plStringView sText, const plVec3& vPosition, const plColor& color, plUInt32 uiSizeInPixel, plEnum<plDebugTextHAlign> horizontalAlignment)
{
  plVec2I32 vPositionInPixel = plVec2I32(static_cast<int>(plMath::Round(vPosition.x)), static_cast<int>(plMath::Round(vPosition.y)));
  plDebugRenderer::Draw2DText(pWorld, sText, vPositionInPixel, color, uiSizeInPixel, horizontalAlignment);
}

// static
void plScriptExtensionClass_Debug::Draw3DText(const plWorld* pWorld, plStringView sText, const plVec3& vPosition, const plColor& color, plUInt32 uiSizeInPixel)
{
  plDebugRenderer::Draw3DText(pWorld, sText, vPosition, color, uiSizeInPixel);
}

// static
void plScriptExtensionClass_Debug::DrawInfoText(const plWorld* pWorld, plStringView sText, plEnum<plDebugTextPlacement> placement, plStringView sGroupName, const plColor& color)
{
  plDebugRenderer::DrawInfoText(pWorld, placement, sGroupName, sText, color);
}

// static
void plScriptExtensionClass_Debug::AddPersistentCross(const plWorld* pWorld, const plVec3& vPosition, float fSize, const plColor& color, const plTransform& transform, plTime duration)
{
  plTransform t = transform;
  t.m_vPosition += vPosition;

  plDebugRenderer::AddPersistentCross(pWorld, fSize, color, t, duration);
}

// static
void plScriptExtensionClass_Debug::AddPersistentLineBox(const plWorld* pWorld, const plVec3& vPosition, const plVec3& vHalfExtents, const plColor& color, const plTransform& transform, plTime duration)
{
  plTransform t = transform;
  t.m_vPosition += vPosition;

  plDebugRenderer::AddPersistentLineBox(pWorld, vHalfExtents, color, t, duration);
}

// static
void plScriptExtensionClass_Debug::AddPersistentLineSphere(const plWorld* pWorld, const plVec3& vPosition, float fRadius, const plColor& color, const plTransform& transform, plTime duration)
{
  plTransform t = transform;
  t.m_vPosition += vPosition;

  plDebugRenderer::AddPersistentLineSphere(pWorld, fRadius, color, t, duration);
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Debug_Implementation_DebugRenderer);
