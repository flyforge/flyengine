#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Transform.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

template <typename Type>
class plRectTemplate;
using plRectFloat = plRectTemplate<float>;

class plFormatString;
class plFrustum;
struct plRenderViewContext;

/// \brief Horizontal alignment of debug text
struct plDebugTextHAlign
{
  using StorageType = plUInt8;

  enum Enum
  {
    Left,
    Center,
    Right,

    Default = Left
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plDebugTextHAlign);

/// \brief Vertical alignment of debug text
struct plDebugTextVAlign
{
  using StorageType = plUInt8;

  enum Enum
  {
    Top,
    Center,
    Bottom,

    Default = Top
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plDebugTextVAlign);

/// \brief Screen placement of debug text
struct plDebugTextPlacement
{
  using StorageType = plUInt8;

  enum Enum
  {
    TopLeft,
    TopCenter,
    TopRight,
    BottomLeft,
    BottomCenter,
    BottomRight,

    ENUM_COUNT,

    Default = TopLeft
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plDebugTextPlacement);

/// \brief Draws simple shapes into the scene or view.
///
/// Shapes can be rendered for a single frame, or 'persistent' for a certain duration.
/// The 'context' specifies whether shapes are generally visible in a scene, from all views,
/// or specific to a single view. See the plDebugRendererContext constructors for what can be implicitly
/// used as a context.
class PLASMA_RENDERERCORE_DLL plDebugRenderer
{
public:
  struct Line
  {
    PLASMA_DECLARE_POD_TYPE();

    Line();
    Line(const plVec3& vStart, const plVec3& vEnd);
    Line(const plVec3& vStart, const plVec3& vEnd, const plColor& color);

    plVec3 m_start;
    plVec3 m_end;

    plColor m_startColor = plColor::White;
    plColor m_endColor = plColor::White;
  };

  struct Triangle
  {
    PLASMA_DECLARE_POD_TYPE();

    Triangle();
    Triangle(const plVec3& v0, const plVec3& v1, const plVec3& v2);

    plVec3 m_position[3];
    plColor m_color = plColor::White;
  };

  struct TexturedTriangle
  {
    PLASMA_DECLARE_POD_TYPE();

    plVec3 m_position[3];
    plVec2 m_texcoord[3];
    plColor m_color = plColor::White;
  };

  /// \brief Renders the given set of lines for one frame.
  static void DrawLines(const plDebugRendererContext& context, plArrayPtr<const Line> lines, const plColor& color, const plTransform& transform = plTransform::MakeIdentity());

  /// \brief Renders the given set of lines in 2D (screen-space) for one frame.
  static void Draw2DLines(const plDebugRendererContext& context, plArrayPtr<const Line> lines, const plColor& color);

  /// \brief Renders a cross for one frame.
  static void DrawCross(const plDebugRendererContext& context, const plVec3& vGlobalPosition, float fLineLength, const plColor& color, const plTransform& transform = plTransform::MakeIdentity());

  /// \brief Renders a wireframe box for one frame.
  static void DrawLineBox(const plDebugRendererContext& context, const plBoundingBox& box, const plColor& color, const plTransform& transform = plTransform::MakeIdentity());

  /// \brief Renders the corners of a wireframe box for one frame.
  static void DrawLineBoxCorners(const plDebugRendererContext& context, const plBoundingBox& box, float fCornerFraction, const plColor& color, const plTransform& transform = plTransform::MakeIdentity());

  /// \brief Renders a wireframe sphere for one frame.
  static void DrawLineSphere(const plDebugRendererContext& context, const plBoundingSphere& sphere, const plColor& color, const plTransform& transform = plTransform::MakeIdentity());

  /// \brief Renders an upright wireframe capsule for one frame.
  static void DrawLineCapsuleZ(const plDebugRendererContext& context, float fLength, float fRadius, const plColor& color, const plTransform& transform = plTransform::MakeIdentity());

  /// \brief Renders a wireframe frustum for one frame.
  static void DrawLineFrustum(const plDebugRendererContext& context, const plFrustum& frustum, const plColor& color, bool bDrawPlaneNormals = false);

  /// \brief Renders a solid box for one frame.
  static void DrawSolidBox(const plDebugRendererContext& context, const plBoundingBox& box, const plColor& color, const plTransform& transform = plTransform::MakeIdentity());

  /// \brief Renders the set of filled triangles for one frame.
  static void DrawSolidTriangles(const plDebugRendererContext& context, plArrayPtr<Triangle> triangles, const plColor& color);

  /// \brief Renders the set of textured triangles for one frame.
  static void DrawTexturedTriangles(const plDebugRendererContext& context, plArrayPtr<TexturedTriangle> triangles, const plColor& color, const plTexture2DResourceHandle& hTexture);

  /// \brief Renders a filled 2D rectangle in screen-space for one frame.
  static void Draw2DRectangle(const plDebugRendererContext& context, const plRectFloat& rectInPixel, float fDepth, const plColor& color);

  /// \brief Renders a textured 2D rectangle in screen-space for one frame.
  static void Draw2DRectangle(const plDebugRendererContext& context, const plRectFloat& rectInPixel, float fDepth, const plColor& color, const plTexture2DResourceHandle& hTexture, plVec2 vScale = plVec2(1, 1));

  /// \brief Renders a textured 2D rectangle in screen-space for one frame.
  static void Draw2DRectangle(const plDebugRendererContext& context, const plRectFloat& rectInPixel, float fDepth, const plColor& color, plGALResourceViewHandle hResourceView, plVec2 vScale = plVec2(1, 1));

  /// \brief Displays a string in screen-space for one frame.
  ///
  /// The string may contain newlines (\n) for multi-line output.
  /// If horizontal alignment is right, the entire text block is aligned according to the longest line.
  /// If vertical alignment is bottom, the entire text block is aligned there.
  ///
  /// Data can be output as a table, by separating columns with tabs (\t). For example:
  /// "| Col 1\t| Col 2\t| Col 3\t|\n| abc\t| 42\t| 11.23\t|"
  ///
  /// Returns the number of lines that the text was split up into.
  static plUInt32 Draw2DText(const plDebugRendererContext& context, const plFormatString& text, const plVec2I32& vPositionInPixel, const plColor& color, plUInt32 uiSizeInPixel = 16, plDebugTextHAlign::Enum horizontalAlignment = plDebugTextHAlign::Left, plDebugTextVAlign::Enum verticalAlignment = plDebugTextVAlign::Top);

  /// \brief Draws a piece of text in one of the screen corners.
  ///
  /// Text positioning is automatic, all lines are placed in each corner such that they don't overlap.
  /// Text from different corners may overlap, though.
  ///
  /// For text formatting options, see Draw2DText().
  ///
  /// The \a groupName parameter is used to insert whitespace between unrelated pieces of text,
  /// it is not displayed anywhere, though.
  ///
  /// Text size cannot be changed.
  static void DrawInfoText(const plDebugRendererContext& context, plDebugTextPlacement::Enum placement, plStringView sGroupName, const plFormatString& text, const plColor& color = plColor::White);

  /// \brief Displays a string in 3D space for one frame.
  static plUInt32 Draw3DText(const plDebugRendererContext& context, const plFormatString& text, const plVec3& vGlobalPosition, const plColor& color, plUInt32 uiSizeInPixel = 16, plDebugTextHAlign::Enum horizontalAlignment = plDebugTextHAlign::Center, plDebugTextVAlign::Enum verticalAlignment = plDebugTextVAlign::Bottom);

  /// \brief Renders a cross at the given location for as many frames until \a duration has passed.
  static void AddPersistentCross(const plDebugRendererContext& context, float fSize, const plColor& color, const plTransform& transform, plTime duration);

  /// \brief Renders a wireframe sphere at the given location for as many frames until \a duration has passed.
  static void AddPersistentLineSphere(const plDebugRendererContext& context, float fRadius, const plColor& color, const plTransform& transform, plTime duration);

  /// \brief Renders a wireframe box at the given location for as many frames until \a duration has passed.
  static void AddPersistentLineBox(const plDebugRendererContext& context, const plVec3& vHalfSize, const plColor& color, const plTransform& transform, plTime duration);

  /// \brief Renders a solid 2D cone in a plane with a given angle.
  ///
  /// The rotation goes around the given \a rotationAxis.
  /// An angle of zero is pointing into forwardAxis direction.
  /// Both angles may be negative.
  static void DrawAngle(const plDebugRendererContext& context, plAngle startAngle, plAngle endAngle, const plColor& solidColor, const plColor& lineColor, const plTransform& transform, plVec3 vForwardAxis = plVec3::MakeAxisX(), plVec3 vRotationAxis = plVec3::MakeAxisZ());

  /// \brief Renders a cone with the tip at the center position, opening up with the given angle.
  static void DrawOpeningCone(const plDebugRendererContext& context, plAngle halfAngle, const plColor& colorInside, const plColor& colorOutside, const plTransform& transform, plVec3 vForwardAxis = plVec3::MakeAxisX());

  /// \brief Renders a bent cone with the tip at the center position, pointing into the +X direction opening up with halfAngle1 and halfAngle2 along the Y and Z axis.
  ///
  /// If solidColor.a > 0, the cone is rendered with as solid triangles.
  /// If lineColor.a > 0, the cone is rendered as lines.
  /// Both can be combined.
  static void DrawLimitCone(const plDebugRendererContext& context, plAngle halfAngle1, plAngle halfAngle2, const plColor& solidColor, const plColor& lineColor, const plTransform& transform);

  /// \brief Renders a cylinder starting at the center position, along the +X axis.
  ///
  /// If the start and end radius are different, a cone or arrow can be created.
  static void DrawCylinder(const plDebugRendererContext& context, float fRadiusStart, float fRadiusEnd, float fLength, const plColor& solidColor, const plColor& lineColor, const plTransform& transform, bool bCapStart = false, bool bCapEnd = false);

private:
  friend class plSimpleRenderPass;

  static void Render(const plRenderViewContext& renderViewContext);
  static void RenderInternal(const plDebugRendererContext& context, const plRenderViewContext& renderViewContext);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, DebugRenderer);
};

/// \brief Helper class to expose debug rendering to scripting
class PLASMA_RENDERERCORE_DLL plScriptExtensionClass_Debug
{
public:
  static void DrawCross(const plWorld* pWorld, const plVec3& vPosition, float fSize, const plColor& color, const plTransform& transform);
  static void DrawLineBox(const plWorld* pWorld, const plVec3& vPosition, const plVec3& vHalfExtents, const plColor& color, const plTransform& transform);
  static void DrawLineSphere(const plWorld* pWorld, const plVec3& vPosition, float fRadius, const plColor& color, const plTransform& transform);

  static void DrawSolidBox(const plWorld* pWorld, const plVec3& vPosition, const plVec3& vHalfExtents, const plColor& color, const plTransform& transform);

  static void Draw2DText(const plWorld* pWorld, plStringView sText, const plVec3& vPositionInPixel, const plColor& color, plUInt32 uiSizeInPixel, plEnum<plDebugTextHAlign> horizontalAlignment);
  static void Draw3DText(const plWorld* pWorld, plStringView sText, const plVec3& vPosition, const plColor& color, plUInt32 uiSizeInPixel);
  static void DrawInfoText(const plWorld* pWorld, plStringView sText, plEnum<plDebugTextPlacement> placement, plStringView sGroupName, const plColor& color);

  static void AddPersistentCross(const plWorld* pWorld, const plVec3& vPosition, float fSize, const plColor& color, const plTransform& transform, plTime duration);
  static void AddPersistentLineBox(const plWorld* pWorld, const plVec3& vPosition, const plVec3& vHalfExtents, const plColor& color, const plTransform& transform, plTime duration);
  static void AddPersistentLineSphere(const plWorld* pWorld, const plVec3& vPosition, float fRadius, const plColor& color, const plTransform& transform, plTime duration);
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plScriptExtensionClass_Debug);

#include <RendererCore/Debug/Implementation/DebugRenderer_inl.h>
