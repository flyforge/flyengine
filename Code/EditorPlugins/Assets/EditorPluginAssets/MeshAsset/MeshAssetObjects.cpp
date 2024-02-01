#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMeshAssetProperties, 3, plRTTIDefaultAllocator<plMeshAssetProperties>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_MEMBER_PROPERTY("PrimitiveType", plMeshPrimitive, m_PrimitiveType),
    PL_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new plFileBrowserAttribute("Select Mesh", plFileBrowserAttribute::Meshes)),
    PL_ENUM_MEMBER_PROPERTY("RightDir", plBasisAxis, m_RightDir)->AddAttributes(new plDefaultValueAttribute((int)plBasisAxis::PositiveX)),
    PL_ENUM_MEMBER_PROPERTY("UpDir", plBasisAxis, m_UpDir)->AddAttributes(new plDefaultValueAttribute((int)plBasisAxis::PositiveY)),
    PL_MEMBER_PROPERTY("FlipForwardDir", m_bFlipForwardDir),
    PL_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0001f, 10000.0f)),
    PL_MEMBER_PROPERTY("RecalculateNormals", m_bRecalculateNormals),
    PL_MEMBER_PROPERTY("RecalculateTangents", m_bRecalculateTrangents)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_ENUM_MEMBER_PROPERTY("NormalPrecision", plMeshNormalPrecision, m_NormalPrecision),
    PL_ENUM_MEMBER_PROPERTY("TexCoordPrecision", plMeshTexCoordPrecision, m_TexCoordPrecision),
    PL_MEMBER_PROPERTY("ImportMaterials", m_bImportMaterials)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, plVariant())),
    PL_MEMBER_PROPERTY("Radius2", m_fRadius2)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, plVariant())),
    PL_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, plVariant())),
    PL_MEMBER_PROPERTY("Detail", m_uiDetail)->AddAttributes(new plDefaultValueAttribute(0), new plClampValueAttribute(0, 128)),
    PL_MEMBER_PROPERTY("Detail2", m_uiDetail2)->AddAttributes(new plDefaultValueAttribute(0), new plClampValueAttribute(0, 128)),
    PL_MEMBER_PROPERTY("Cap", m_bCap)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("Cap2", m_bCap2)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(360.0f)), new plClampValueAttribute(plAngle::MakeFromDegree(0.0f), plAngle::MakeFromDegree(360.0f))),
    PL_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new plContainerAttribute(false, true, true)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class plMeshAssetPropertiesPatch_1_2 : public plGraphPatch
{
public:
  plMeshAssetPropertiesPatch_1_2()
    : plGraphPatch("plMeshAssetProperties", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Primitive Type", "PrimitiveType");
    pNode->RenameProperty("Forward Dir", "ForwardDir");
    pNode->RenameProperty("Right Dir", "RightDir");
    pNode->RenameProperty("Up Dir", "UpDir");
    pNode->RenameProperty("Uniform Scaling", "UniformScaling");
    pNode->RenameProperty("Non-Uniform Scaling", "NonUniformScaling");
    pNode->RenameProperty("Mesh File", "MeshFile");
    pNode->RenameProperty("Radius 2", "Radius2");
    pNode->RenameProperty("Detail 2", "Detail2");
    pNode->RenameProperty("Cap 2", "Cap2");
    pNode->RenameProperty("Import Materials", "ImportMaterials");
  }
};

plMeshAssetPropertiesPatch_1_2 g_MeshAssetPropertiesPatch_1_2;

PL_BEGIN_STATIC_REFLECTED_ENUM(plMeshPrimitive, 1)
  PL_ENUM_CONSTANT(plMeshPrimitive::File), PL_ENUM_CONSTANT(plMeshPrimitive::Box), PL_ENUM_CONSTANT(plMeshPrimitive::Rect), PL_ENUM_CONSTANT(plMeshPrimitive::Cylinder), PL_ENUM_CONSTANT(plMeshPrimitive::Cone), PL_ENUM_CONSTANT(plMeshPrimitive::Pyramid), PL_ENUM_CONSTANT(plMeshPrimitive::Sphere), PL_ENUM_CONSTANT(plMeshPrimitive::HalfSphere), PL_ENUM_CONSTANT(plMeshPrimitive::GeodesicSphere), PL_ENUM_CONSTANT(plMeshPrimitive::Capsule), PL_ENUM_CONSTANT(plMeshPrimitive::Torus),
PL_END_STATIC_REFLECTED_ENUM;

plMeshAssetProperties::plMeshAssetProperties() = default;
plMeshAssetProperties::~plMeshAssetProperties() = default;


void plMeshAssetProperties::PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == plGetStaticRTTI<plMeshAssetProperties>())
  {
    plInt64 primType = e.m_pObject->GetTypeAccessor().GetValue("PrimitiveType").ConvertTo<plInt64>();

    auto& props = *e.m_pPropertyStates;

    props["MeshFile"].m_Visibility = plPropertyUiState::Invisible;
    props["Radius"].m_Visibility = plPropertyUiState::Invisible;
    props["Radius2"].m_Visibility = plPropertyUiState::Invisible;
    props["Height"].m_Visibility = plPropertyUiState::Invisible;
    props["Detail"].m_Visibility = plPropertyUiState::Invisible;
    props["Detail2"].m_Visibility = plPropertyUiState::Invisible;
    props["Cap"].m_Visibility = plPropertyUiState::Invisible;
    props["Cap2"].m_Visibility = plPropertyUiState::Invisible;
    props["Angle"].m_Visibility = plPropertyUiState::Invisible;
    props["ImportMaterials"].m_Visibility = plPropertyUiState::Invisible;
    props["RecalculateNormals"].m_Visibility = plPropertyUiState::Invisible;
    props["RecalculateTangents"].m_Visibility = plPropertyUiState::Invisible;
    props["NormalPrecision"].m_Visibility = plPropertyUiState::Invisible;
    props["TexCoordPrecision"].m_Visibility = plPropertyUiState::Invisible;

    switch (primType)
    {
      case plMeshPrimitive::File:
        props["MeshFile"].m_Visibility = plPropertyUiState::Default;
        props["ImportMaterials"].m_Visibility = plPropertyUiState::Default;
        props["RecalculateNormals"].m_Visibility = plPropertyUiState::Default;
        props["RecalculateTangents"].m_Visibility = plPropertyUiState::Default;
        props["NormalPrecision"].m_Visibility = plPropertyUiState::Default;
        props["TexCoordPrecision"].m_Visibility = plPropertyUiState::Default;
        break;

      case plMeshPrimitive::Box:
        break;

      case plMeshPrimitive::Rect:
        props["Detail"].m_Visibility = plPropertyUiState::Default;
        props["Detail2"].m_Visibility = plPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Rect.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Rect.Detail2";
        break;

      case plMeshPrimitive::Capsule:
        props["Radius"].m_Visibility = plPropertyUiState::Default;
        props["Height"].m_Visibility = plPropertyUiState::Default;
        props["Detail"].m_Visibility = plPropertyUiState::Default;
        props["Detail2"].m_Visibility = plPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Sphere.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Sphere.Detail2";
        break;

      case plMeshPrimitive::Cone:
        props["Radius"].m_Visibility = plPropertyUiState::Default;
        props["Height"].m_Visibility = plPropertyUiState::Default;
        props["Detail"].m_Visibility = plPropertyUiState::Default;
        props["Cap"].m_Visibility = plPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Cylinder.Detail";
        break;

      case plMeshPrimitive::Cylinder:
        props["Radius"].m_Visibility = plPropertyUiState::Default;
        props["Radius2"].m_Visibility = plPropertyUiState::Default;
        props["Height"].m_Visibility = plPropertyUiState::Default;
        props["Detail"].m_Visibility = plPropertyUiState::Default;
        props["Cap"].m_Visibility = plPropertyUiState::Default;
        props["Cap2"].m_Visibility = plPropertyUiState::Default;
        props["Angle"].m_Visibility = plPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Cylinder.Detail";
        props["Radius"].m_sNewLabelText = "Prim.Cylinder.Radius1";
        props["Radius2"].m_sNewLabelText = "Prim.Cylinder.Radius2";
        props["Angle"].m_sNewLabelText = "Prim.Cylinder.Angle";
        props["Cap"].m_sNewLabelText = "Prim.Cylinder.Cap1";
        props["Cap2"].m_sNewLabelText = "Prim.Cylinder.Cap2";
        break;

      case plMeshPrimitive::GeodesicSphere:
        props["Radius"].m_Visibility = plPropertyUiState::Default;
        props["Detail"].m_Visibility = plPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.GeoSphere.Detail";
        break;

      case plMeshPrimitive::HalfSphere:
        props["Radius"].m_Visibility = plPropertyUiState::Default;
        props["Detail"].m_Visibility = plPropertyUiState::Default;
        props["Detail2"].m_Visibility = plPropertyUiState::Default;
        props["Cap"].m_Visibility = plPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Sphere.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Sphere.Detail2";
        break;

      case plMeshPrimitive::Pyramid:
        props["Cap"].m_Visibility = plPropertyUiState::Default;
        break;

      case plMeshPrimitive::Sphere:
        props["Radius"].m_Visibility = plPropertyUiState::Default;
        props["Detail"].m_Visibility = plPropertyUiState::Default;
        props["Detail2"].m_Visibility = plPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Sphere.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Sphere.Detail2";
        break;

      case plMeshPrimitive::Torus:
        props["Radius"].m_Visibility = plPropertyUiState::Default;
        props["Radius2"].m_Visibility = plPropertyUiState::Default;
        props["Detail"].m_Visibility = plPropertyUiState::Default;
        props["Detail2"].m_Visibility = plPropertyUiState::Default;

        props["Detail"].m_sNewLabelText = "Prim.Torus.Detail1";
        props["Detail2"].m_sNewLabelText = "Prim.Torus.Detail2";
        props["Radius"].m_sNewLabelText = "Prim.Torus.Radius1";
        props["Radius2"].m_sNewLabelText = "Prim.Torus.Radius2";
        break;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

class plMeshAssetPropertiesPatch_2_3 : public plGraphPatch
{
public:
  plMeshAssetPropertiesPatch_2_3()
    : plGraphPatch("plMeshAssetProperties", 3)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    // convert the "Angle" property from float to plAngle
    if (auto pProp = pNode->FindProperty("Angle"))
    {
      if (pProp->m_Value.IsA<float>())
      {
        const float valFloat = pProp->m_Value.Get<float>();
        pProp->m_Value = plAngle::MakeFromDegree(valFloat);
      }
    }
  }
};

plMeshAssetPropertiesPatch_2_3 g_plMeshAssetPropertiesPatch_2_3;
