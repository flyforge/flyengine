#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plJoltSurfaceResourceSlot, plNoBase, 1, plRTTIDefaultAllocator<plJoltSurfaceResourceSlot>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new plReadOnlyAttribute()),
    PLASMA_MEMBER_PROPERTY("Resource", m_sResource)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface")),
    PLASMA_MEMBER_PROPERTY("Exclude", m_bExclude),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plJoltCollisionMeshType, 2)
  PLASMA_ENUM_CONSTANT(plJoltCollisionMeshType::ConvexHull),
  PLASMA_ENUM_CONSTANT(plJoltCollisionMeshType::TriangleMesh),
  PLASMA_ENUM_CONSTANT(plJoltCollisionMeshType::Cylinder),
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plJoltConvexCollisionMeshType, 1)
  PLASMA_ENUM_CONSTANT(plJoltConvexCollisionMeshType::ConvexHull),
  PLASMA_ENUM_CONSTANT(plJoltConvexCollisionMeshType::Cylinder),
  PLASMA_ENUM_CONSTANT(plJoltConvexCollisionMeshType::ConvexDecomposition),
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plJoltCollisionMeshAssetProperties, 1, plRTTIDefaultAllocator<plJoltCollisionMeshAssetProperties>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("RightDir", plBasisAxis, m_RightDir)->AddAttributes(new plDefaultValueAttribute((int)plBasisAxis::PositiveX)),
    PLASMA_ENUM_MEMBER_PROPERTY("UpDir", plBasisAxis, m_UpDir)->AddAttributes(new plDefaultValueAttribute((int)plBasisAxis::PositiveY)),
    PLASMA_MEMBER_PROPERTY("FlipForwardDir", m_bFlipForwardDir),
    PLASMA_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("IsConvexMesh", m_bIsConvexMesh)->AddAttributes(new plHiddenAttribute()),
    PLASMA_ENUM_MEMBER_PROPERTY("ConvexMeshType", plJoltConvexCollisionMeshType, m_ConvexMeshType),
    PLASMA_MEMBER_PROPERTY("MaxConvexPieces", m_uiMaxConvexPieces)->AddAttributes(new plDefaultValueAttribute(5)),
    PLASMA_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_MEMBER_PROPERTY("Radius2", m_fRadius2)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_MEMBER_PROPERTY("Detail", m_uiDetail)->AddAttributes(new plDefaultValueAttribute(1), new plClampValueAttribute(0, 32)),
    PLASMA_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new plFileBrowserAttribute("Select Mesh", plFileBrowserAttribute::Meshes)),
    PLASMA_ARRAY_MEMBER_PROPERTY("Surfaces", m_Slots)->AddAttributes(new plContainerAttribute(false, false, true)),
    PLASMA_MEMBER_PROPERTY("Surface", m_sConvexMeshSurface)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface")),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltCollisionMeshAssetProperties::plJoltCollisionMeshAssetProperties() = default;
plJoltCollisionMeshAssetProperties::~plJoltCollisionMeshAssetProperties() = default;

void plJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() != plGetStaticRTTI<plJoltCollisionMeshAssetProperties>())
    return;

  const bool isConvex = e.m_pObject->GetTypeAccessor().GetValue("IsConvexMesh").ConvertTo<bool>();
  const plInt64 meshType = e.m_pObject->GetTypeAccessor().GetValue("ConvexMeshType").ConvertTo<plInt64>();

  auto& props = *e.m_pPropertyStates;

  props["Radius"].m_Visibility = plPropertyUiState::Invisible;
  props["Radius2"].m_Visibility = plPropertyUiState::Invisible;
  props["Height"].m_Visibility = plPropertyUiState::Invisible;
  props["Detail"].m_Visibility = plPropertyUiState::Invisible;
  props["MeshFile"].m_Visibility = plPropertyUiState::Invisible;
  props["ConvexMeshType"].m_Visibility = plPropertyUiState::Invisible;
  props["MaxConvexPieces"].m_Visibility = plPropertyUiState::Invisible;
  props["Surfaces"].m_Visibility = isConvex ? plPropertyUiState::Invisible : plPropertyUiState::Default;
  props["Surface"].m_Visibility = isConvex ? plPropertyUiState::Default : plPropertyUiState::Invisible;

  if (!isConvex)
  {
    props["MeshFile"].m_Visibility = plPropertyUiState::Default;
  }
  else
  {
    props["ConvexMeshType"].m_Visibility = plPropertyUiState::Default;

    switch (meshType)
    {
      case plJoltConvexCollisionMeshType::ConvexDecomposition:
        props["MaxConvexPieces"].m_Visibility = plPropertyUiState::Default;
        [[fallthrough]];

      case plJoltConvexCollisionMeshType::ConvexHull:
        props["MeshFile"].m_Visibility = plPropertyUiState::Default;
        break;

      case plJoltConvexCollisionMeshType::Cylinder:
        props["Radius"].m_Visibility = plPropertyUiState::Default;
        props["Radius2"].m_Visibility = plPropertyUiState::Default;
        props["Height"].m_Visibility = plPropertyUiState::Default;
        props["Detail"].m_Visibility = plPropertyUiState::Default;
        break;
    }
  }
}
