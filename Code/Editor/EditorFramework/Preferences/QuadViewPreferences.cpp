#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Preferences/QuadViewPreferences.h>
#include <Foundation/Serialization/GraphPatch.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plEngineViewPreferences, plNoBase, 2, plRTTIDefaultAllocator<plEngineViewPreferences>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("CamPos", m_vCamPos),
    PLASMA_MEMBER_PROPERTY("CamDir", m_vCamDir),
    PLASMA_MEMBER_PROPERTY("CamUp", m_vCamUp),
    PLASMA_ENUM_MEMBER_PROPERTY("Perspective", plSceneViewPerspective, m_PerspectiveMode),
    PLASMA_ENUM_MEMBER_PROPERTY("RenderMode", plViewRenderMode, m_RenderMode),
    PLASMA_MEMBER_PROPERTY("FOV", m_fFov),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// Patch class
  class plSceneViewPreferencesPatch_1_2 : public plGraphPatch
  {
  public:
    plSceneViewPreferencesPatch_1_2()
      : plGraphPatch("plSceneViewPreferences", 2)
    {
    }
    virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
    {
      ref_context.RenameClass("plEngineViewPreferences");
    }
  };
  plSceneViewPreferencesPatch_1_2 g_plSceneViewPreferencesPatch_1_2;
} // namespace

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plQuadViewPreferencesUser, 1, plRTTIDefaultAllocator<plQuadViewPreferencesUser>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("QuadView", m_bQuadView)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("ViewSingle", m_ViewSingle)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("ViewQuad0", m_ViewQuad0)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("ViewQuad1", m_ViewQuad1)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("ViewQuad2", m_ViewQuad2)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("ViewQuad3", m_ViewQuad3)->AddAttributes(new plHiddenAttribute()),
    PLASMA_ARRAY_ACCESSOR_PROPERTY("FavoriteCams", FavCams_GetCount, FavCams_GetCam, FavCams_SetCam, FavCams_Insert, FavCams_Remove)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plQuadViewPreferencesUser::plQuadViewPreferencesUser()
  : plPreferences(Domain::Document, "View")
{
  m_bQuadView = false;

  m_ViewSingle.m_vCamPos.Set(-3, 0, 2);
  m_ViewSingle.m_vCamDir.Set(1, 0, -0.5f);
  m_ViewSingle.m_vCamDir.Normalize();
  m_ViewSingle.m_vCamUp = m_ViewSingle.m_vCamDir.CrossRH(plVec3(0, 1, 0)).GetNormalized();
  m_ViewSingle.m_PerspectiveMode = plSceneViewPerspective::Perspective;
  m_ViewSingle.m_RenderMode = plViewRenderMode::Default;
  m_ViewSingle.m_fFov = 70.0f;

  // Top Left: Top Down
  m_ViewQuad0.m_vCamPos.SetZero();
  m_ViewQuad0.m_vCamDir.Set(0, 0, -1);
  m_ViewQuad0.m_vCamUp.Set(1, 0, 0);
  m_ViewQuad0.m_PerspectiveMode = plSceneViewPerspective::Orthogonal_Top;
  m_ViewQuad0.m_RenderMode = plViewRenderMode::WireframeMonochrome;
  m_ViewQuad0.m_fFov = 20.0f;

  // Top Right: Perspective
  m_ViewQuad1.m_vCamPos = m_ViewSingle.m_vCamPos;
  m_ViewQuad1.m_vCamDir = m_ViewSingle.m_vCamDir;
  m_ViewQuad1.m_vCamUp = m_ViewSingle.m_vCamUp;
  m_ViewQuad1.m_PerspectiveMode = plSceneViewPerspective::Perspective;
  m_ViewQuad1.m_RenderMode = plViewRenderMode::Default;
  m_ViewQuad1.m_fFov = 70.0f;

  // Bottom Left: Front to Back
  m_ViewQuad2.m_vCamPos.SetZero();
  m_ViewQuad2.m_vCamDir.Set(-1, 0, 0);
  m_ViewQuad2.m_vCamUp.Set(0, 0, 1);
  m_ViewQuad2.m_PerspectiveMode = plSceneViewPerspective::Orthogonal_Front;
  m_ViewQuad2.m_RenderMode = plViewRenderMode::WireframeMonochrome;
  m_ViewQuad2.m_fFov = 20.0f;

  // Bottom Right: Right to Left
  m_ViewQuad3.m_vCamPos.SetZero();
  m_ViewQuad3.m_vCamDir.Set(0, -1, 0);
  m_ViewQuad3.m_vCamUp.Set(0, 0, 1);
  m_ViewQuad3.m_PerspectiveMode = plSceneViewPerspective::Orthogonal_Right;
  m_ViewQuad3.m_RenderMode = plViewRenderMode::WireframeMonochrome;
  m_ViewQuad3.m_fFov = 20.0f;
}
