#include <CppProjectPlugin/CppProjectPluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <CppProjectPlugin/Components/SampleRenderComponent.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_BITFLAGS(SampleRenderComponentMask, 1)
  PLASMA_BITFLAGS_CONSTANT(SampleRenderComponentMask::Box),
  PLASMA_BITFLAGS_CONSTANT(SampleRenderComponentMask::Sphere),
  PLASMA_BITFLAGS_CONSTANT(SampleRenderComponentMask::Cross),
  PLASMA_BITFLAGS_CONSTANT(SampleRenderComponentMask::Quad)
PLASMA_END_STATIC_REFLECTED_BITFLAGS;

PLASMA_BEGIN_COMPONENT_TYPE(SampleRenderComponent, 1 /* version */, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new plDefaultValueAttribute(1), new plClampValueAttribute(0, 10)),
    PLASMA_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new plDefaultValueAttribute(plColor::White)),
    PLASMA_ACCESSOR_PROPERTY("Texture", GetTextureFile, SetTextureFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    PLASMA_BITFLAGS_MEMBER_PROPERTY("Render", SampleRenderComponentMask, m_RenderTypes)->AddAttributes(new plDefaultValueAttribute(SampleRenderComponentMask::Box)),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("CppProject"), // Component menu group
  }
  PLASMA_END_ATTRIBUTES;

  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgSetColor, OnSetColor)
  }
  PLASMA_END_MESSAGEHANDLERS;

  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(SetRandomColor)
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

SampleRenderComponent::SampleRenderComponent() = default;
SampleRenderComponent::~SampleRenderComponent() = default;

void SampleRenderComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fSize;
  s << m_Color;
  s << m_hTexture;
  s << m_RenderTypes;
}

void SampleRenderComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fSize;
  s >> m_Color;
  s >> m_hTexture;
  s >> m_RenderTypes;
}

void SampleRenderComponent::SetTexture(const plTexture2DResourceHandle& hTexture)
{
  m_hTexture = hTexture;
}

const plTexture2DResourceHandle& SampleRenderComponent::GetTexture() const
{
  return m_hTexture;
}

void SampleRenderComponent::SetTextureFile(const char* szFile)
{
  plTexture2DResourceHandle hTexture;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hTexture = plResourceManager::LoadResource<plTexture2DResource>(szFile);
  }

  SetTexture(hTexture);
}

const char* SampleRenderComponent::GetTextureFile(void) const
{
  if (m_hTexture.IsValid())
    return m_hTexture.GetResourceID();

  return nullptr;
}

void SampleRenderComponent::OnSetColor(plMsgSetColor& msg)
{
  m_Color = msg.m_Color;
}

void SampleRenderComponent::SetRandomColor()
{
  plRandom& rng = GetWorld()->GetRandomNumberGenerator();

  m_Color.r = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
  m_Color.g = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
  m_Color.b = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
}

void SampleRenderComponent::Update()
{
  const plTransform ownerTransform = GetOwner()->GetGlobalTransform();

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Box))
  {
    plBoundingBox bbox;
    bbox.SetCenterAndHalfExtents(plVec3::ZeroVector(), plVec3(m_fSize));

    plDebugRenderer::DrawLineBox(GetWorld(), bbox, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Cross))
  {
    plDebugRenderer::DrawCross(GetWorld(), plVec3::ZeroVector(), m_fSize, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Sphere))
  {
    plBoundingSphere sphere;
    sphere.SetElements(plVec3::ZeroVector(), m_fSize);
    plDebugRenderer::DrawLineSphere(GetWorld(), sphere, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Quad) && m_hTexture.IsValid())
  {
    plHybridArray<plDebugRenderer::TexturedTriangle, 16> triangles;

    {
      auto& t0 = triangles.ExpandAndGetRef();

      t0.m_position[0].Set(0, -m_fSize, +m_fSize);
      t0.m_position[1].Set(0, +m_fSize, -m_fSize);
      t0.m_position[2].Set(0, -m_fSize, -m_fSize);

      t0.m_texcoord[0].Set(0.0f, 0.0f);
      t0.m_texcoord[1].Set(1.0f, 1.0f);
      t0.m_texcoord[2].Set(0.0f, 1.0f);
    }

    {
      auto& t1 = triangles.ExpandAndGetRef();

      t1.m_position[0].Set(0, -m_fSize, +m_fSize);
      t1.m_position[1].Set(0, +m_fSize, +m_fSize);
      t1.m_position[2].Set(0, +m_fSize, -m_fSize);

      t1.m_texcoord[0].Set(0.0f, 0.0f);
      t1.m_texcoord[1].Set(1.0f, 0.0f);
      t1.m_texcoord[2].Set(1.0f, 1.0f);
    }

    // move the triangles into our object space
    for (auto& tri : triangles)
    {
      tri.m_position[0] = ownerTransform.TransformPosition(tri.m_position[0]);
      tri.m_position[1] = ownerTransform.TransformPosition(tri.m_position[1]);
      tri.m_position[2] = ownerTransform.TransformPosition(tri.m_position[2]);
    }

    plDebugRenderer::DrawTexturedTriangles(GetWorld(), triangles, m_Color, m_hTexture);
  }
}
