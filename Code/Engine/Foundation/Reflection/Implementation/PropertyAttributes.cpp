#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Reflection.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plPropertyAttribute, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plReadOnlyAttribute, 1, plRTTIDefaultAllocator<plReadOnlyAttribute>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plHiddenAttribute, 1, plRTTIDefaultAllocator<plHiddenAttribute>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTemporaryAttribute, 1, plRTTIDefaultAllocator<plTemporaryAttribute>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_BITFLAGS(plDependencyFlags, 1)
  PL_BITFLAGS_CONSTANTS(plDependencyFlags::Package, plDependencyFlags::Thumbnail, plDependencyFlags::Transform)
PL_END_STATIC_REFLECTED_BITFLAGS;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCategoryAttribute, 1, plRTTIDefaultAllocator<plCategoryAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Category", m_sCategory),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plInDevelopmentAttribute, 1, plRTTIDefaultAllocator<plInDevelopmentAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Phase", m_Phase),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plInt32),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;


const char* plInDevelopmentAttribute::GetString() const
{
  switch (m_Phase)
  {
  case Phase::Alpha:
    return "ALPHA";

  case Phase::Beta:
    return "BETA";

    PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return "";
}

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTitleAttribute, 1, plRTTIDefaultAllocator<plTitleAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Title", m_sTitle),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plColorAttribute, 1, plRTTIDefaultAllocator<plColorAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Color", m_Color),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plColor),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plExposeColorAlphaAttribute, 1, plRTTIDefaultAllocator<plExposeColorAlphaAttribute>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSuffixAttribute, 1, plRTTIDefaultAllocator<plSuffixAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Suffix", m_sSuffix),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMinValueTextAttribute, 1, plRTTIDefaultAllocator<plMinValueTextAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Text", m_sText),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDefaultValueAttribute, 1, plRTTIDefaultAllocator<plDefaultValueAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Value", m_Value),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const plVariant&),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plImageSliderUiAttribute, 1, plRTTIDefaultAllocator<plImageSliderUiAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ImageGenerator", m_sImageGenerator),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plClampValueAttribute, 1, plRTTIDefaultAllocator<plClampValueAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Min", m_MinValue),
    PL_MEMBER_PROPERTY("Max", m_MaxValue),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const plVariant&, const plVariant&),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGroupAttribute, 1, plRTTIDefaultAllocator<plGroupAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Group", m_sGroup),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*, float),
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*, float),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

plGroupAttribute::plGroupAttribute()
= default;

plGroupAttribute::plGroupAttribute(const char* szGroup, float fOrder)
  : m_sGroup(szGroup)
  , m_fOrder(fOrder)
{
}

plGroupAttribute::plGroupAttribute(const char* szGroup, const char* szIconName, float fOrder)
  : m_sGroup(szGroup)
  , m_sIconName(szIconName)
  , m_fOrder(fOrder)
{
}

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeWidgetAttribute, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plContainerWidgetAttribute, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTagSetWidgetAttribute, 1, plRTTIDefaultAllocator<plTagSetWidgetAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Filter", m_sTagFilter),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plNoTemporaryTransactionsAttribute, 1, plRTTIDefaultAllocator<plNoTemporaryTransactionsAttribute>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plExposedParametersAttribute, 1, plRTTIDefaultAllocator<plExposedParametersAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ParametersSource", m_sParametersSource),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDynamicDefaultValueAttribute, 1, plRTTIDefaultAllocator<plDynamicDefaultValueAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ClassSource", m_sClassSource),
    PL_MEMBER_PROPERTY("ClassType", m_sClassType),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plContainerAttribute, 1, plRTTIDefaultAllocator<plContainerAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("CanAdd", m_bCanAdd),
    PL_MEMBER_PROPERTY("CanDelete", m_bCanDelete),
    PL_MEMBER_PROPERTY("CanMove", m_bCanMove),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(bool, bool, bool),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plFileBrowserAttribute, 1, plRTTIDefaultAllocator<plFileBrowserAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Title", m_sDialogTitle),
    PL_MEMBER_PROPERTY("Filter", m_sTypeFilter),
    PL_MEMBER_PROPERTY("CustomAction", m_sCustomAction),
    PL_BITFLAGS_MEMBER_PROPERTY("DependencyFlags", plDependencyFlags, m_DependencyFlags),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plStringView, plStringView),
    PL_CONSTRUCTOR_PROPERTY(plStringView, plStringView, plStringView),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAssetBrowserAttribute, 1, plRTTIDefaultAllocator<plAssetBrowserAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Filter", m_sTypeFilter),
    PL_BITFLAGS_MEMBER_PROPERTY("DependencyFlags", plDependencyFlags, m_DependencyFlags),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDynamicEnumAttribute, 1, plRTTIDefaultAllocator<plDynamicEnumAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
   PL_MEMBER_PROPERTY("DynamicEnum", m_sDynamicEnumName),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
   PL_CONSTRUCTOR_PROPERTY(const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDynamicStringEnumAttribute, 1, plRTTIDefaultAllocator<plDynamicStringEnumAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("DynamicEnum", m_sDynamicEnumName),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDynamicBitflagsAttribute, 1, plRTTIDefaultAllocator<plDynamicBitflagsAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
   PL_MEMBER_PROPERTY("DynamicBitflags", m_sDynamicBitflagsName),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
   PL_CONSTRUCTOR_PROPERTY(plStringView),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plManipulatorAttribute, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Property1", m_sProperty1),
    PL_MEMBER_PROPERTY("Property2", m_sProperty2),
    PL_MEMBER_PROPERTY("Property3", m_sProperty3),
    PL_MEMBER_PROPERTY("Property4", m_sProperty4),
    PL_MEMBER_PROPERTY("Property5", m_sProperty5),
    PL_MEMBER_PROPERTY("Property6", m_sProperty6),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plManipulatorAttribute::plManipulatorAttribute(const char* szProperty1, const char* szProperty2 /*= nullptr*/, const char* szProperty3 /*= nullptr*/,
  const char* szProperty4 /*= nullptr*/, const char* szProperty5 /*= nullptr*/, const char* szProperty6 /*= nullptr*/)
  : m_sProperty1(szProperty1)
  , m_sProperty2(szProperty2)
  , m_sProperty3(szProperty3)
  , m_sProperty4(szProperty4)
  , m_sProperty5(szProperty5)
  , m_sProperty6(szProperty6)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSphereManipulatorAttribute, 1, plRTTIDefaultAllocator<plSphereManipulatorAttribute>)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*),
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSphereManipulatorAttribute::plSphereManipulatorAttribute()
  : plManipulatorAttribute(nullptr)
{
}

plSphereManipulatorAttribute::plSphereManipulatorAttribute(const char* szOuterRadius, const char* szInnerRadius)
  : plManipulatorAttribute(szOuterRadius, szInnerRadius)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCapsuleManipulatorAttribute, 1, plRTTIDefaultAllocator<plCapsuleManipulatorAttribute>)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCapsuleManipulatorAttribute::plCapsuleManipulatorAttribute()
  : plManipulatorAttribute(nullptr)
{
}

plCapsuleManipulatorAttribute::plCapsuleManipulatorAttribute(const char* szLength, const char* szRadius)
  : plManipulatorAttribute(szLength, szRadius)
{
}


//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plBoxManipulatorAttribute, 1, plRTTIDefaultAllocator<plBoxManipulatorAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("scale", m_fSizeScale),
    PL_MEMBER_PROPERTY("recenter", m_bRecenterParent),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*, bool, float),
    PL_CONSTRUCTOR_PROPERTY(const char*, bool, float),
    PL_CONSTRUCTOR_PROPERTY(const char*, bool, float, const char*),
    PL_CONSTRUCTOR_PROPERTY(const char*, bool, float, const char*, const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plBoxManipulatorAttribute::plBoxManipulatorAttribute()
  : plManipulatorAttribute(nullptr)
{
}

plBoxManipulatorAttribute::plBoxManipulatorAttribute(const char* szSizeProperty, float fSizeScale, bool bRecenterParent, const char* szOffsetProperty, const char* szRotationProperty)
  : plManipulatorAttribute(szSizeProperty, szOffsetProperty, szRotationProperty)
{
  m_bRecenterParent = bRecenterParent;
  m_fSizeScale = fSizeScale;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plNonUniformBoxManipulatorAttribute, 1, plRTTIDefaultAllocator<plNonUniformBoxManipulatorAttribute>)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const char*, const char*, const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plNonUniformBoxManipulatorAttribute::plNonUniformBoxManipulatorAttribute()
  : plManipulatorAttribute(nullptr)
{
}

plNonUniformBoxManipulatorAttribute::plNonUniformBoxManipulatorAttribute(
  const char* szNegXProp, const char* szPosXProp, const char* szNegYProp, const char* szPosYProp, const char* szNegZProp, const char* szPosZProp)
  : plManipulatorAttribute(szNegXProp, szPosXProp, szNegYProp, szPosYProp, szNegZProp, szPosZProp)
{
}

plNonUniformBoxManipulatorAttribute::plNonUniformBoxManipulatorAttribute(const char* szSizeX, const char* szSizeY, const char* szSizeZ)
  : plManipulatorAttribute(szSizeX, szSizeY, szSizeZ)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plConeLengthManipulatorAttribute, 1, plRTTIDefaultAllocator<plConeLengthManipulatorAttribute>)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plConeLengthManipulatorAttribute::plConeLengthManipulatorAttribute()
  : plManipulatorAttribute(nullptr)
{
}

plConeLengthManipulatorAttribute::plConeLengthManipulatorAttribute(const char* szRadiusProperty)
  : plManipulatorAttribute(szRadiusProperty)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plConeAngleManipulatorAttribute, 1, plRTTIDefaultAllocator<plConeAngleManipulatorAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("scale", m_fScale),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*),
    PL_CONSTRUCTOR_PROPERTY(const char*, float),
    PL_CONSTRUCTOR_PROPERTY(const char*, float, const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plConeAngleManipulatorAttribute::plConeAngleManipulatorAttribute()
  : plManipulatorAttribute(nullptr)
{
  m_fScale = 1.0f;
}

plConeAngleManipulatorAttribute::plConeAngleManipulatorAttribute(const char* szAngleProperty, float fScale, const char* szRadiusProperty)
  : plManipulatorAttribute(szAngleProperty, szRadiusProperty)
{
  m_fScale = fScale;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTransformManipulatorAttribute, 1, plRTTIDefaultAllocator<plTransformManipulatorAttribute>)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plTransformManipulatorAttribute::plTransformManipulatorAttribute()
  : plManipulatorAttribute(nullptr)
{
}

plTransformManipulatorAttribute::plTransformManipulatorAttribute(
  const char* szTranslateProperty, const char* szRotateProperty, const char* szScaleProperty, const char* szOffsetTranslation, const char* szOffsetRotation)
  : plManipulatorAttribute(szTranslateProperty, szRotateProperty, szScaleProperty, szOffsetTranslation, szOffsetRotation)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plBoneManipulatorAttribute, 1, plRTTIDefaultAllocator<plBoneManipulatorAttribute>)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plBoneManipulatorAttribute::plBoneManipulatorAttribute()
  : plManipulatorAttribute(nullptr)
{
}

plBoneManipulatorAttribute::plBoneManipulatorAttribute(const char* szTransformProperty, const char* szBindTo)
  : plManipulatorAttribute(szTransformProperty, szBindTo)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_BITFLAGS(plVisualizerAnchor, 1)
  PL_BITFLAGS_CONSTANTS(plVisualizerAnchor::Center, plVisualizerAnchor::PosX, plVisualizerAnchor::NegX, plVisualizerAnchor::PosY, plVisualizerAnchor::NegY, plVisualizerAnchor::PosZ, plVisualizerAnchor::NegZ)
PL_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plVisualizerAttribute, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Property1", m_sProperty1),
    PL_MEMBER_PROPERTY("Property2", m_sProperty2),
    PL_MEMBER_PROPERTY("Property3", m_sProperty3),
    PL_MEMBER_PROPERTY("Property4", m_sProperty4),
    PL_MEMBER_PROPERTY("Property5", m_sProperty5),
    PL_BITFLAGS_MEMBER_PROPERTY("Anchor", plVisualizerAnchor, m_Anchor),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plVisualizerAttribute::plVisualizerAttribute(const char* szProperty1, const char* szProperty2 /*= nullptr*/, const char* szProperty3 /*= nullptr*/,
  const char* szProperty4 /*= nullptr*/, const char* szProperty5 /*= nullptr*/)
  : m_sProperty1(szProperty1)
  , m_sProperty2(szProperty2)
  , m_sProperty3(szProperty3)
  , m_sProperty4(szProperty4)
  , m_sProperty5(szProperty5)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plBoxVisualizerAttribute, 1, plRTTIDefaultAllocator<plBoxVisualizerAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Color", m_Color),
    PL_MEMBER_PROPERTY("OffsetOrScale", m_vOffsetOrScale),
    PL_MEMBER_PROPERTY("SizeScale", m_fSizeScale),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*, float, const plColor&, const char*, plBitflags<plVisualizerAnchor>, plVec3, const char*, const char*),
    PL_CONSTRUCTOR_PROPERTY(const char*, float, const plColor&, const char*, plBitflags<plVisualizerAnchor>, plVec3, const char*),
    PL_CONSTRUCTOR_PROPERTY(const char*, float, const plColor&, const char*, plBitflags<plVisualizerAnchor>, plVec3),
    PL_CONSTRUCTOR_PROPERTY(const char*, float, const plColor&, const char*, plBitflags<plVisualizerAnchor>),
    PL_CONSTRUCTOR_PROPERTY(const char*, float, const plColor&, const char*),
    PL_CONSTRUCTOR_PROPERTY(const char*, float, const plColor&),
    PL_CONSTRUCTOR_PROPERTY(const char*, float),
    PL_CONSTRUCTOR_PROPERTY(const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plBoxVisualizerAttribute::plBoxVisualizerAttribute()
  : plVisualizerAttribute(nullptr)
{
}

plBoxVisualizerAttribute::plBoxVisualizerAttribute(const char* szSizeProperty, float fSizeScale, const plColor& fixedColor /*= plColorScheme::LightUI(plColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, plBitflags<plVisualizerAnchor> anchor /*= plVisualizerAnchor::Center*/, plVec3 vOffsetOrScale /*= plVec3::MakeZero*/, const char* szOffsetProperty /*= nullptr*/, const char* szRotationProperty /*= nullptr*/)
  : plVisualizerAttribute(szSizeProperty, szColorProperty, szOffsetProperty, szRotationProperty)
  , m_Color(fixedColor)
  , m_vOffsetOrScale(vOffsetOrScale)
{
  m_Anchor = anchor;
  m_fSizeScale = fSizeScale;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSphereVisualizerAttribute, 1, plRTTIDefaultAllocator<plSphereVisualizerAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Color", m_Color),
    PL_MEMBER_PROPERTY("OffsetOrScale", m_vOffsetOrScale),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*, const plColor&, const char*, plBitflags<plVisualizerAnchor>, plVec3, const char*),
    PL_CONSTRUCTOR_PROPERTY(const char*, const plColor&, const char*, plBitflags<plVisualizerAnchor>, plVec3),
    PL_CONSTRUCTOR_PROPERTY(const char*, const plColor&, const char*, plBitflags<plVisualizerAnchor>),
    PL_CONSTRUCTOR_PROPERTY(const char*, const plColor&, const char*),
    PL_CONSTRUCTOR_PROPERTY(const char*, const plColor&),
    PL_CONSTRUCTOR_PROPERTY(const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSphereVisualizerAttribute::plSphereVisualizerAttribute()
  : plVisualizerAttribute(nullptr)
{
}

plSphereVisualizerAttribute::plSphereVisualizerAttribute(const char* szRadiusProperty, const plColor& fixedColor /*= plColorScheme::LightUI(plColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, plBitflags<plVisualizerAnchor> anchor /*= plVisualizerAnchor::Center*/, plVec3 vOffsetOrScale /*= plVec3::MakeZero*/, const char* szOffsetProperty /*= nullptr*/)
  : plVisualizerAttribute(szRadiusProperty, szColorProperty, szOffsetProperty)
  , m_Color(fixedColor)
  , m_vOffsetOrScale(vOffsetOrScale)
{
  m_Anchor = anchor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCapsuleVisualizerAttribute, 1, plRTTIDefaultAllocator<plCapsuleVisualizerAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Color", m_Color),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*, const plColor&, const char*, plBitflags<plVisualizerAnchor>),
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*, const plColor&, const char*),
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*, const plColor&),
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCapsuleVisualizerAttribute::plCapsuleVisualizerAttribute()
  : plVisualizerAttribute(nullptr)
{
}

plCapsuleVisualizerAttribute::plCapsuleVisualizerAttribute(const char* szHeightProperty, const char* szRadiusProperty, const plColor& fixedColor /*= plColorScheme::LightUI(plColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, plBitflags<plVisualizerAnchor> anchor /*= plVisualizerAnchor::Center*/)
  : plVisualizerAttribute(szHeightProperty, szRadiusProperty, szColorProperty)
  , m_Color(fixedColor)
{
  m_Anchor = anchor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCylinderVisualizerAttribute, 1, plRTTIDefaultAllocator<plCylinderVisualizerAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Color", m_Color),
    PL_MEMBER_PROPERTY("OffsetOrScale", m_vOffsetOrScale),
    PL_ENUM_MEMBER_PROPERTY("Axis", plBasisAxis, m_Axis),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plEnum<plBasisAxis>, const char*, const char*, const plColor&, const char*, plBitflags<plVisualizerAnchor>, plVec3, const char*),
    PL_CONSTRUCTOR_PROPERTY(plEnum<plBasisAxis>, const char*, const char*, const plColor&, const char*, plBitflags<plVisualizerAnchor>, plVec3),
    PL_CONSTRUCTOR_PROPERTY(plEnum<plBasisAxis>, const char*, const char*, const plColor&, const char*, plBitflags<plVisualizerAnchor>),
    PL_CONSTRUCTOR_PROPERTY(plEnum<plBasisAxis>, const char*, const char*, const plColor&, const char*),
    PL_CONSTRUCTOR_PROPERTY(plEnum<plBasisAxis>, const char*, const char*, const plColor&),
    PL_CONSTRUCTOR_PROPERTY(plEnum<plBasisAxis>, const char*, const char*),
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const plColor&, const char*, plBitflags<plVisualizerAnchor>, plVec3, const char*),
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const plColor&, const char*, plBitflags<plVisualizerAnchor>, plVec3),
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const plColor&, const char*, plBitflags<plVisualizerAnchor>),
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const plColor&, const char*),
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const plColor&),
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCylinderVisualizerAttribute::plCylinderVisualizerAttribute()
  : plVisualizerAttribute(nullptr)
{
}

plCylinderVisualizerAttribute::plCylinderVisualizerAttribute(plEnum<plBasisAxis> axis, const char* szHeightProperty, const char* szRadiusProperty, const plColor& fixedColor /*= plColorScheme::LightUI(plColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, plBitflags<plVisualizerAnchor> anchor /*= plVisualizerAnchor::Center*/, plVec3 vOffsetOrScale /*= plVec3::MakeZero*/, const char* szOffsetProperty /*= nullptr*/)
  : plVisualizerAttribute(szHeightProperty, szRadiusProperty, szColorProperty, szOffsetProperty)
  , m_Color(fixedColor)
  , m_vOffsetOrScale(vOffsetOrScale)
  , m_Axis(axis)
{
  m_Anchor = anchor;
}

plCylinderVisualizerAttribute::plCylinderVisualizerAttribute(const char* szAxisProperty, const char* szHeightProperty, const char* szRadiusProperty, const plColor& fixedColor /*= plColorScheme::LightUI(plColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, plBitflags<plVisualizerAnchor> anchor /*= plVisualizerAnchor::Center*/, plVec3 vOffsetOrScale /*= plVec3::MakeZero()*/, const char* szOffsetProperty /*= nullptr*/)
  : plVisualizerAttribute(szHeightProperty, szRadiusProperty, szColorProperty, szOffsetProperty, szAxisProperty)
  , m_Color(fixedColor)
  , m_vOffsetOrScale(vOffsetOrScale)
{
  m_Axis = plBasisAxis::Default;
  m_Anchor = anchor;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDirectionVisualizerAttribute, 1, plRTTIDefaultAllocator<plDirectionVisualizerAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_MEMBER_PROPERTY("Axis", plBasisAxis, m_Axis),
    PL_MEMBER_PROPERTY("Color", m_Color),
    PL_MEMBER_PROPERTY("Scale", m_fScale)
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plEnum<plBasisAxis>, float, const plColor&, const char*, const char*),
    PL_CONSTRUCTOR_PROPERTY(plEnum<plBasisAxis>, float, const plColor&, const char*),
    PL_CONSTRUCTOR_PROPERTY(plEnum<plBasisAxis>, float, const plColor&),
    PL_CONSTRUCTOR_PROPERTY(plEnum<plBasisAxis>, float),
    PL_CONSTRUCTOR_PROPERTY(const char*, float, const plColor&, const char*, const char*),
    PL_CONSTRUCTOR_PROPERTY(const char*, float, const plColor&, const char*),
    PL_CONSTRUCTOR_PROPERTY(const char*, float, const plColor&),
    PL_CONSTRUCTOR_PROPERTY(const char*, float),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plDirectionVisualizerAttribute::plDirectionVisualizerAttribute()
  : plVisualizerAttribute(nullptr)
{
  m_Axis = plBasisAxis::PositiveX;
  m_fScale = 1.0f;
  m_Color = plColor::White;
}

plDirectionVisualizerAttribute::plDirectionVisualizerAttribute(plEnum<plBasisAxis> axis, float fScale, const plColor& fixedColor /*= plColorScheme::LightUI(plColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, const char* szLengthProperty /*= nullptr*/)
  : plVisualizerAttribute(szColorProperty, szLengthProperty)
  , m_Axis(axis)
  , m_Color(fixedColor)
  , m_fScale(fScale)
{
}

plDirectionVisualizerAttribute::plDirectionVisualizerAttribute(const char* szAxisProperty, float fScale, const plColor& fixedColor /*= plColorScheme::LightUI(plColorScheme::Grape)*/, const char* szColorProperty /*= nullptr*/, const char* szLengthProperty /*= nullptr*/)
  : plVisualizerAttribute(szColorProperty, szLengthProperty, szAxisProperty)
  , m_Axis(plBasisAxis::PositiveX)
  , m_Color(fixedColor)
  , m_fScale(fScale)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plConeVisualizerAttribute, 1, plRTTIDefaultAllocator<plConeVisualizerAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_MEMBER_PROPERTY("Axis", plBasisAxis, m_Axis),
    PL_MEMBER_PROPERTY("Color", m_Color),
    PL_MEMBER_PROPERTY("Scale", m_fScale),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plEnum<plBasisAxis>, const char*, float, const char*, const plColor&, const char*),
    PL_CONSTRUCTOR_PROPERTY(plEnum<plBasisAxis>, const char*, float, const char*, const plColor&),
    PL_CONSTRUCTOR_PROPERTY(plEnum<plBasisAxis>, const char*, float, const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plConeVisualizerAttribute::plConeVisualizerAttribute()
  : plVisualizerAttribute(nullptr)
  , m_Axis(plBasisAxis::PositiveX)
  , m_Color(plColor::Red)
  , m_fScale(1.0f)
{
}

plConeVisualizerAttribute::plConeVisualizerAttribute(plEnum<plBasisAxis> axis, const char* szAngleProperty, float fScale,
  const char* szRadiusProperty, const plColor& fixedColor /*= plColorScheme::LightUI(plColorScheme::Grape)*/, const char* szColorProperty)
  : plVisualizerAttribute(szAngleProperty, szRadiusProperty, szColorProperty)
  , m_Axis(axis)
  , m_Color(fixedColor)
  , m_fScale(fScale)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCameraVisualizerAttribute, 1, plRTTIDefaultAllocator<plCameraVisualizerAttribute>)
{
  //PL_BEGIN_PROPERTIES
  //PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*, const char*, const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCameraVisualizerAttribute::plCameraVisualizerAttribute()
  : plVisualizerAttribute(nullptr)
{
}

plCameraVisualizerAttribute::plCameraVisualizerAttribute(const char* szModeProperty, const char* szFovProperty, const char* szOrthoDimProperty,
  const char* szNearPlaneProperty, const char* szFarPlaneProperty)
  : plVisualizerAttribute(szModeProperty, szFovProperty, szOrthoDimProperty, szNearPlaneProperty, szFarPlaneProperty)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMaxArraySizeAttribute, 1, plRTTIDefaultAllocator<plMaxArraySizeAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("MaxSize", m_uiMaxSize),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(plUInt32),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plPreventDuplicatesAttribute, 1, plRTTIDefaultAllocator<plPreventDuplicatesAttribute>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plExcludeFromScript, 1, plRTTIDefaultAllocator<plExcludeFromScript>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plScriptableFunctionAttribute, 1, plRTTIDefaultAllocator<plScriptableFunctionAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("ArgNames", m_ArgNames),
    PL_ARRAY_MEMBER_PROPERTY("ArgTypes", m_ArgTypes),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plScriptableFunctionAttribute::plScriptableFunctionAttribute(ArgType argType1 /*= In*/, const char* szArg1 /*= nullptr*/, ArgType argType2 /*= In*/,
  const char* szArg2 /*= nullptr*/, ArgType argType3 /*= In*/, const char* szArg3 /*= nullptr*/, ArgType argType4 /*= In*/,
  const char* szArg4 /*= nullptr*/, ArgType argType5 /*= In*/, const char* szArg5 /*= nullptr*/, ArgType argType6 /*= In*/,
  const char* szArg6 /*= nullptr*/)
{
  {
    if (plStringUtils::IsNullOrEmpty(szArg1))
      return;

    m_ArgNames.PushBack(szArg1);
    m_ArgTypes.PushBack(argType1);
  }
  {
    if (plStringUtils::IsNullOrEmpty(szArg2))
      return;

    m_ArgNames.PushBack(szArg2);
    m_ArgTypes.PushBack(argType2);
  }
  {
    if (plStringUtils::IsNullOrEmpty(szArg3))
      return;

    m_ArgNames.PushBack(szArg3);
    m_ArgTypes.PushBack(argType3);
  }
  {
    if (plStringUtils::IsNullOrEmpty(szArg4))
      return;

    m_ArgNames.PushBack(szArg4);
    m_ArgTypes.PushBack(argType4);
  }
  {
    if (plStringUtils::IsNullOrEmpty(szArg5))
      return;

    m_ArgNames.PushBack(szArg5);
    m_ArgTypes.PushBack(argType5);
  }
  {
    if (plStringUtils::IsNullOrEmpty(szArg6))
      return;

    m_ArgNames.PushBack(szArg6);
    m_ArgTypes.PushBack(argType6);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plFunctionArgumentAttributes, 1, plRTTIDefaultAllocator<plFunctionArgumentAttributes>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ArgIndex", m_uiArgIndex),
    PL_ARRAY_MEMBER_PROPERTY("ArgAttributes", m_ArgAttributes),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plFunctionArgumentAttributes::plFunctionArgumentAttributes(plUInt32 uiArgIndex, const plPropertyAttribute* pAttribute1, const plPropertyAttribute* pAttribute2 /*= nullptr*/, const plPropertyAttribute* pAttribute3 /*= nullptr*/, const plPropertyAttribute* pAttribute4 /*= nullptr*/)
  : m_uiArgIndex(uiArgIndex)
{
  {
    if (pAttribute1 == nullptr)
      return;

    m_ArgAttributes.PushBack(pAttribute1);
  }
  {
    if (pAttribute2 == nullptr)
      return;

    m_ArgAttributes.PushBack(pAttribute2);
  }
  {
    if (pAttribute3 == nullptr)
      return;

    m_ArgAttributes.PushBack(pAttribute3);
  }
  {
    if (pAttribute4 == nullptr)
      return;

    m_ArgAttributes.PushBack(pAttribute4);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDynamicPinAttribute, 1, plRTTIDefaultAllocator<plDynamicPinAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Property", m_sProperty)
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plDynamicPinAttribute::plDynamicPinAttribute(const char* szProperty)
  : m_sProperty(szProperty)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLongOpAttribute, 1, plRTTIDefaultAllocator<plLongOpAttribute>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Type", m_sOpTypeName),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(),
    PL_CONSTRUCTOR_PROPERTY(const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameObjectReferenceAttribute, 1, plRTTIDefaultAllocator<plGameObjectReferenceAttribute>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGroupNextAttribute, 1, plRTTIDefaultAllocator<plGroupNextAttribute>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


//////////////////////////////////////////////////////////////////////////

PL_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_PropertyAttributes);
