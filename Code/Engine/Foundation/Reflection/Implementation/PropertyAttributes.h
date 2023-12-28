#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Reflection/Reflection.h>

/// \brief Base class of all attributes can be used to decorate a RTTI property.
class PLASMA_FOUNDATION_DLL plPropertyAttribute : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPropertyAttribute, plReflectedClass);
};

/// \brief A property attribute that indicates that the property may not be modified through the UI
class PLASMA_FOUNDATION_DLL plReadOnlyAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plReadOnlyAttribute, plPropertyAttribute);
};

/// \brief A property attribute that indicates that the property is not to be shown in the UI
class PLASMA_FOUNDATION_DLL plHiddenAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plHiddenAttribute, plPropertyAttribute);
};

/// \brief A property attribute that indicates that the property is not to be serialized
/// and whatever it points to only exists temporarily while running or in editor.
class PLASMA_FOUNDATION_DLL plTemporaryAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTemporaryAttribute, plPropertyAttribute);
};

/// \brief Used to categorize types (e.g. add component menu)
class PLASMA_FOUNDATION_DLL plCategoryAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCategoryAttribute, plPropertyAttribute);

public:
  plCategoryAttribute() = default;
  plCategoryAttribute(const char* szCategory)
    : m_sCategory(szCategory)
  {
  }

  const char* GetCategory() const { return m_sCategory; }

private:
  plUntrackedString m_sCategory;
};

/// \brief A property attribute that indicates that this feature is still in development and should not be shown to all users.
class PLASMA_FOUNDATION_DLL plInDevelopmentAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plInDevelopmentAttribute, plPropertyAttribute);

public:
  enum Phase
  {
    Alpha,
    Beta
  };

  plInDevelopmentAttribute() = default;
  plInDevelopmentAttribute(plInt32 iPhase) { m_Phase = iPhase; }

  const char* GetString() const;

  plInt32 m_Phase = Phase::Beta;
};


/// \brief Used for dynamic titles of visual script nodes.
/// E.g. "Set Bool Property '{Name}'" will allow the title to by dynamic
/// by reading the current value of the 'Name' property.
class PLASMA_FOUNDATION_DLL plTitleAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTitleAttribute, plPropertyAttribute);

public:
  plTitleAttribute() = default;
  plTitleAttribute(const char* szTitle)
    : m_sTitle(szTitle)
  {
  }

  const char* GetTitle() const { return m_sTitle; }

private:
  plUntrackedString m_sTitle;
};

/// \brief Used to colorize types
class PLASMA_FOUNDATION_DLL plColorAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plColorAttribute, plPropertyAttribute);

public:
  plColorAttribute() = default;
  plColorAttribute(const plColor& color)
    : m_Color(color)
  {
  }

  const plColor& GetColor() const { return m_Color; }

private:
  plColor m_Color;
};

/// \brief A property attribute that indicates that the alpha channel of an plColorGammaUB or plColor should be exposed in the UI.
class PLASMA_FOUNDATION_DLL plExposeColorAlphaAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plExposeColorAlphaAttribute, plPropertyAttribute);
};

/// \brief Used for any property shown as a line edit (int, float, vector etc).
class PLASMA_FOUNDATION_DLL plSuffixAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSuffixAttribute, plPropertyAttribute);

public:
  plSuffixAttribute() = default;
  plSuffixAttribute(const char* szSuffix)
    : m_sSuffix(szSuffix)
  {
  }

  const char* GetSuffix() const { return m_sSuffix; }

private:
  plUntrackedString m_sSuffix;
};

/// \brief Used to show a text instead of the minimum value of a property.
class PLASMA_FOUNDATION_DLL plMinValueTextAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMinValueTextAttribute, plPropertyAttribute);

public:
  plMinValueTextAttribute() = default;
  plMinValueTextAttribute(const char* szText)
    : m_sText(szText)
  {
  }

  const char* GetText() const { return m_sText; }

private:
  plUntrackedString m_sText;
};

/// \brief Sets the default value of the property.
class PLASMA_FOUNDATION_DLL plDefaultValueAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDefaultValueAttribute, plPropertyAttribute);

public:
  plDefaultValueAttribute() = default;

  plDefaultValueAttribute(const plVariant& value)
    : m_Value(value)
  {
  }

  plDefaultValueAttribute(plInt32 value)
    : m_Value(value)
  {
  }

  plDefaultValueAttribute(float value)
    : m_Value(value)
  {
  }

  plDefaultValueAttribute(double value)
    : m_Value(value)
  {
  }

  plDefaultValueAttribute(plStringView value)
    : m_Value(plVariant(value, false))
  {
  }

  plDefaultValueAttribute(const char* value)
    : m_Value(plVariant(plStringView(value), false))
  {
  }

  const plVariant& GetValue() const { return m_Value; }

private:
  plVariant m_Value;
};

/// \brief A property attribute that allows to define min and max values for the UI. Min or max may be set to an invalid variant to indicate
/// unbounded values in one direction.
class PLASMA_FOUNDATION_DLL plClampValueAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plClampValueAttribute, plPropertyAttribute);

public:
  plClampValueAttribute() = default;
  plClampValueAttribute(const plVariant& min, const plVariant& max)
    : m_MinValue(min)
    , m_MaxValue(max)
  {
  }

  const plVariant& GetMinValue() const { return m_MinValue; }
  const plVariant& GetMaxValue() const { return m_MaxValue; }

private:
  plVariant m_MinValue;
  plVariant m_MaxValue;
};

/// \brief Used to categorize properties into groups
class PLASMA_FOUNDATION_DLL plGroupAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGroupAttribute, plPropertyAttribute);

public:
  plGroupAttribute();
  plGroupAttribute(const char* szGroup, float fOrder = -1.0f);
  plGroupAttribute(const char* szGroup, const char* szIconName, float fOrder = -1.0f);

  const char* GetGroup() const { return m_sGroup; }
  const char* GetIconName() const { return m_sIconName; }
  float GetOrder() const { return m_fOrder; }

private:
  plUntrackedString m_sGroup;
  plUntrackedString m_sIconName;
  float m_fOrder = -1.0f;
};

/// \brief Derive from this class if you want to define an attribute that replaces the property type widget.
///
/// Using this attribute affects both member properties as well as elements in a container but not the container widget.
/// When creating a property widget, the property grid will look for an attribute of this type and use
/// its type to look for a factory creator in plRttiMappedObjectFactory<plQtPropertyWidget>.
/// E.g. plRttiMappedObjectFactory<plQtPropertyWidget>::RegisterCreator(plGetStaticRTTI<plFileBrowserAttribute>(), FileBrowserCreator);
/// will replace the property widget for all properties that use plFileBrowserAttribute.
class PLASMA_FOUNDATION_DLL plTypeWidgetAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTypeWidgetAttribute, plPropertyAttribute);
};

/// \brief Derive from this class if you want to define an attribute that replaces the property widget of containers.
///
/// Using this attribute affects the container widget but not container elements.
/// Only derive from this class if you want to replace the container widget itself, in every other case
/// prefer to use plTypeWidgetAttribute.
class PLASMA_FOUNDATION_DLL plContainerWidgetAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plContainerWidgetAttribute, plPropertyAttribute);
};

/// \brief Add this attribute to a tag set member property to make it use the tag set editor
/// and define the categories it will use as a ; separated list of category names.
///
/// Usage: PLASMA_SET_MEMBER_PROPERTY("Tags", m_Tags)->AddAttributes(new plTagSetWidgetAttribute("Category1;Category2")),
class PLASMA_FOUNDATION_DLL plTagSetWidgetAttribute : public plContainerWidgetAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTagSetWidgetAttribute, plContainerWidgetAttribute);

public:
  plTagSetWidgetAttribute() = default;
  plTagSetWidgetAttribute(const char* szTagFilter)
    : m_sTagFilter(szTagFilter)
  {
  }

  const char* GetTagFilter() const { return m_sTagFilter; }

private:
  plUntrackedString m_sTagFilter;
};

/// \brief This attribute indicates that a widget should not use temporary transactions when changing the value.
class PLASMA_FOUNDATION_DLL plNoTemporaryTransactionsAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plNoTemporaryTransactionsAttribute, plPropertyAttribute);
};

/// \brief Add this attribute to a variant map property to make it map to the exposed parameters
/// of an asset. For this, the member property name of the asset reference needs to be passed in.
/// The exposed parameters of the currently set asset on that property will be used as the source.
///
/// Usage:
/// PLASMA_ACCESSOR_PROPERTY("Effect", GetParticleEffectFile, SetParticleEffectFile)->AddAttributes(new plAssetBrowserAttribute("Particle
/// Effect")), PLASMA_MAP_ACCESSOR_PROPERTY("Parameters",...)->AddAttributes(new plExposedParametersAttribute("Effect")),
class PLASMA_FOUNDATION_DLL plExposedParametersAttribute : public plContainerWidgetAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plExposedParametersAttribute, plContainerWidgetAttribute);

public:
  plExposedParametersAttribute() = default;
  plExposedParametersAttribute(const char* szParametersSource)
    : m_sParametersSource(szParametersSource)
  {
  }

  const char* GetParametersSource() const { return m_sParametersSource; }

private:
  plUntrackedString m_sParametersSource;
};

/// \brief Add this attribute to an embedded class or container property to make it retrieve its default values from a dynamic meta info object on an asset.
///
/// The default values are retrieved from the asset meta data of the currently set asset on that property.
///
/// Usage:
/// PLASMA_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new plAssetBrowserAttribute("Skeleton")),
///
/// // Use this if the embedded class m_SkeletonMetaData is of type plSkeletonMetaData.
/// PLASMA_MEMBER_PROPERTY("SkeletonMetaData", m_SkeletonMetaData)->AddAttributes(new plDynamicDefaultValueAttribute("Skeleton", "plSkeletonMetaData")),
///
/// // Use this if you don't want embed the entire meta object but just some container of it. In this case the LocalBones container must match in type to the property 'BonesArrayNameInMetaData' in the meta data type 'plSkeletonMetaData'.
/// PLASMA_MAP_MEMBER_PROPERTY("LocalBones", m_Bones)->AddAttributes(new plDynamicDefaultValueAttribute("Skeleton", "plSkeletonMetaData", "BonesArrayNameInMetaData")),
class PLASMA_FOUNDATION_DLL plDynamicDefaultValueAttribute : public plTypeWidgetAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDynamicDefaultValueAttribute, plTypeWidgetAttribute);

public:
  plDynamicDefaultValueAttribute() = default;
  plDynamicDefaultValueAttribute(const char* szClassSource,
    const char* szClassType, const char* szClassProperty = nullptr)
    : m_sClassSource(szClassSource)
    , m_sClassType(szClassType)
    , m_sClassProperty(szClassProperty)
  {
  }

  const char* GetClassSource() const { return m_sClassSource; }
  const char* GetClassType() const { return m_sClassType; }
  const char* GetClassProperty() const { return m_sClassProperty; }

private:
  plUntrackedString m_sClassSource;
  plUntrackedString m_sClassType;
  plUntrackedString m_sClassProperty;
};


/// \brief Sets the allowed actions on a container.
class PLASMA_FOUNDATION_DLL plContainerAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plContainerAttribute, plPropertyAttribute);

public:
  plContainerAttribute() = default;
  plContainerAttribute(bool bCanAdd, bool bCanDelete, bool bCanMove)
  {
    m_bCanAdd = bCanAdd;
    m_bCanDelete = bCanDelete;
    m_bCanMove = bCanMove;
  }

  bool CanAdd() const { return m_bCanAdd; }
  bool CanDelete() const { return m_bCanDelete; }
  bool CanMove() const { return m_bCanMove; }

private:
  bool m_bCanAdd = false;
  bool m_bCanDelete = false;
  bool m_bCanMove = false;
};

/// \brief Defines how a reference set by plFileBrowserAttribute and plAssetBrowserAttribute is treated.
///
/// A few examples to explain the flags:
/// ## Input for a mesh: **Transform | Thumbnail**
/// * The input (e.g. fbx) is obviously needed for transforming the asset.
/// * We also can't generate a thumbnail without it.
/// * But we don't need to package it with the final game as it is not used by the runtime.
///
/// ## Material on a mesh: **Thumbnail | Package**
/// * The default material on a mesh asset is not needed to transform the mesh. As only the material reference is stored in the mesh asset, any changes to the material do not affect the transform output of the mesh.
/// * It is obviously needed for the thumbnail as that is what is displayed in it.
/// * We also need to package this reference as otherwise the runtime would fail to instantiate the mesh without errors.
///
/// ## Surface on hit prefab: **Package**
/// * Transforming a surface is not affected if the prefab it spawns on impact changes. Only the reference is stored.
/// * The set prefab does not show up in the thumbnail so it is not needed.
/// * We do however need to package it or otherwise the runtime would fail to spawn the prefab on impact.
///
/// As a rule of thumb (also the default for each):
/// * plFileBrowserAttribute are mostly Transform and Thumbnail.
/// * plAssetBrowserAttribute are mostly Thumbnail and Package.
struct plDependencyFlags
{
  using StorageType = plUInt8;

  enum Enum
  {
    None = 0,              ///< The reference is not needed for anything in production. An example of this is editor references that are only used at edit time, e.g. a default animation clip for a skeleton.
    Thumbnail = PLASMA_BIT(0), ///< This reference is a dependency to generating a thumbnail. The material references of a mesh for example.
    Transform = PLASMA_BIT(1), ///< This reference is a dependency to transforming this asset. The input model of a mesh for example.
    Package = PLASMA_BIT(2),   ///< This reference is needs to be packaged as it is used at runtime by this asset. All sounds or debris generated on impact of a surface are common examples of this.
    Default = 0
  };

  struct Bits
  {
    StorageType Thumbnail : 1;
    StorageType Transform : 1;
    StorageType Package : 1;
  };
};

PLASMA_DECLARE_FLAGS_OPERATORS(plDependencyFlags);
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_FOUNDATION_DLL, plDependencyFlags);

/// \brief A property attribute that indicates that the string property should display a file browsing button.
///
/// Allows to specify the title for the browse dialog and the allowed file types.
/// Usage: PLASMA_MEMBER_PROPERTY("File", m_sFilePath)->AddAttributes(new plFileBrowserAttribute("Choose a File", "*.txt")),
class PLASMA_FOUNDATION_DLL plFileBrowserAttribute : public plTypeWidgetAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plFileBrowserAttribute, plTypeWidgetAttribute);

public:
  // Predefined common type filters
  static constexpr const char* Meshes = "*.obj;*.fbx;*.gltf;*.glb";
  static constexpr const char* MeshesWithAnimations = "*.fbx;*.gltf;*.glb";
  static constexpr const char* ImagesLdrOnly = "*.dds;*.tga;*.png;*.jpg;*.jpeg";
  static constexpr const char* ImagesHdrOnly = "*.hdr;*.exr";
  static constexpr const char* ImagesLdrAndHdr = "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr;*.exr";
  static constexpr const char* CubemapsLdrAndHdr = "*.dds;*.hdr";

  plFileBrowserAttribute() = default;
  plFileBrowserAttribute(const char* szDialogTitle,
    const char* szTypeFilter, const char* szCustomAction = nullptr,
    plBitflags<plDependencyFlags> depencyFlags = plDependencyFlags::Transform | plDependencyFlags::Thumbnail)
    : m_sDialogTitle(szDialogTitle)
    , m_sTypeFilter(szTypeFilter)
    , m_sCustomAction(szCustomAction)
    , m_DependencyFlags(depencyFlags)
  {
  }

  const char* GetDialogTitle() const { return m_sDialogTitle; }
  const char* GetTypeFilter() const { return m_sTypeFilter; }
  const char* GetCustomAction() const { return m_sCustomAction; }
  plBitflags<plDependencyFlags> GetDependencyFlags() const { return m_DependencyFlags; }

private:
  plUntrackedString m_sDialogTitle;
  plUntrackedString m_sTypeFilter;
  plUntrackedString m_sCustomAction;
  plBitflags<plDependencyFlags> m_DependencyFlags;
};

/// \brief A property attribute that indicates that the string property is actually an asset reference.
///
/// Allows to specify the allowed asset types, separated with ;
/// Usage: PLASMA_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new plAssetBrowserAttribute("Texture 2D;Texture 3D")),
class PLASMA_FOUNDATION_DLL plAssetBrowserAttribute : public plTypeWidgetAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAssetBrowserAttribute, plTypeWidgetAttribute);

public:
  plAssetBrowserAttribute() = default;
  plAssetBrowserAttribute(const char* szTypeFilter,
    plBitflags<plDependencyFlags> depencyFlags = plDependencyFlags::Thumbnail | plDependencyFlags::Package)
    : m_DependencyFlags(depencyFlags)
  {
    SetTypeFilter(szTypeFilter);
  }

  void SetTypeFilter(const char* szTypeFilter)
  {
    plStringBuilder sTemp(";", szTypeFilter, ";");
    m_sTypeFilter = sTemp;
  }
  const char* GetTypeFilter() const { return m_sTypeFilter; }
  plBitflags<plDependencyFlags> GetDependencyFlags() const { return m_DependencyFlags; }

private:
  plUntrackedString m_sTypeFilter;
  plBitflags<plDependencyFlags> m_DependencyFlags;
};

/// \brief Can be used on integer properties to display them as enums. The valid enum values and their names may change at runtime.
///
/// See plDynamicEnum for details.
class PLASMA_FOUNDATION_DLL plDynamicEnumAttribute : public plTypeWidgetAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDynamicEnumAttribute, plTypeWidgetAttribute);

public:
  plDynamicEnumAttribute() = default;
  plDynamicEnumAttribute(const char* szDynamicEnumName)
    : m_sDynamicEnumName(szDynamicEnumName)
  {
  }

  const char* GetDynamicEnumName() const { return m_sDynamicEnumName; }

private:
  plUntrackedString m_sDynamicEnumName;
};

/// \brief Can be used on string properties to display them as enums. The valid enum values and their names may change at runtime.
///
/// See plDynamicStringEnum for details.
class PLASMA_FOUNDATION_DLL plDynamicStringEnumAttribute : public plTypeWidgetAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDynamicStringEnumAttribute, plTypeWidgetAttribute);

public:
  plDynamicStringEnumAttribute() = default;
  plDynamicStringEnumAttribute(const char* szDynamicEnumName)
    : m_sDynamicEnumName(szDynamicEnumName)
  {
  }

  const char* GetDynamicEnumName() const { return m_sDynamicEnumName; }

private:
  plUntrackedString m_sDynamicEnumName;
};

/// \brief Can be used on integer properties to display them as bitflags. The valid bitflags and their names may change at runtime.
class PLASMA_FOUNDATION_DLL plDynamicBitflagsAttribute : public plTypeWidgetAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDynamicBitflagsAttribute, plTypeWidgetAttribute);

public:
  plDynamicBitflagsAttribute() = default;
  plDynamicBitflagsAttribute(plStringView sDynamicName)
    : m_sDynamicBitflagsName(sDynamicName)
  {
  }

  plStringView GetDynamicBitflagsName() const { return m_sDynamicBitflagsName; }

private:
  plUntrackedString m_sDynamicBitflagsName;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plManipulatorAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plManipulatorAttribute, plPropertyAttribute);

public:
  plManipulatorAttribute(const char* szProperty1, const char* szProperty2 = nullptr, const char* szProperty3 = nullptr,
    const char* szProperty4 = nullptr, const char* szProperty5 = nullptr, const char* szProperty6 = nullptr);

  plUntrackedString m_sProperty1;
  plUntrackedString m_sProperty2;
  plUntrackedString m_sProperty3;
  plUntrackedString m_sProperty4;
  plUntrackedString m_sProperty5;
  plUntrackedString m_sProperty6;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plSphereManipulatorAttribute : public plManipulatorAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSphereManipulatorAttribute, plManipulatorAttribute);

public:
  plSphereManipulatorAttribute();
  plSphereManipulatorAttribute(const char* szOuterRadiusProperty, const char* szInnerRadiusProperty = nullptr);

  const plUntrackedString& GetOuterRadiusProperty() const { return m_sProperty1; }
  const plUntrackedString& GetInnerRadiusProperty() const { return m_sProperty2; }
};


//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plCapsuleManipulatorAttribute : public plManipulatorAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCapsuleManipulatorAttribute, plManipulatorAttribute);

public:
  plCapsuleManipulatorAttribute();
  plCapsuleManipulatorAttribute(const char* szHeightProperty, const char* szRadiusProperty);

  const plUntrackedString& GetLengthProperty() const { return m_sProperty1; }
  const plUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
};


//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plBoxManipulatorAttribute : public plManipulatorAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plBoxManipulatorAttribute, plManipulatorAttribute);

public:
  plBoxManipulatorAttribute();
  plBoxManipulatorAttribute(const char* szSizeProperty, float fSizeScale, bool bRecenterParent, const char* szOffsetProperty = nullptr, const char* szRotationProperty = nullptr);

  bool m_bRecenterParent = false;
  float m_fSizeScale = 1.0f;

  const plUntrackedString& GetSizeProperty() const { return m_sProperty1; }
  const plUntrackedString& GetOffsetProperty() const { return m_sProperty2; }
  const plUntrackedString& GetRotationProperty() const { return m_sProperty3; }
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plNonUniformBoxManipulatorAttribute : public plManipulatorAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plNonUniformBoxManipulatorAttribute, plManipulatorAttribute);

public:
  plNonUniformBoxManipulatorAttribute();
  plNonUniformBoxManipulatorAttribute(
    const char* szNegXProp, const char* szPosXProp, const char* szNegYProp, const char* szPosYProp, const char* szNegZProp, const char* szPosZProp);
  plNonUniformBoxManipulatorAttribute(const char* szSizeX, const char* szSizeY, const char* szSizeZ);

  bool HasSixAxis() const { return !m_sProperty4.IsEmpty(); }

  const plUntrackedString& GetNegXProperty() const { return m_sProperty1; }
  const plUntrackedString& GetPosXProperty() const { return m_sProperty2; }
  const plUntrackedString& GetNegYProperty() const { return m_sProperty3; }
  const plUntrackedString& GetPosYProperty() const { return m_sProperty4; }
  const plUntrackedString& GetNegZProperty() const { return m_sProperty5; }
  const plUntrackedString& GetPosZProperty() const { return m_sProperty6; }

  const plUntrackedString& GetSizeXProperty() const { return m_sProperty1; }
  const plUntrackedString& GetSizeYProperty() const { return m_sProperty2; }
  const plUntrackedString& GetSizeZProperty() const { return m_sProperty3; }
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plConeLengthManipulatorAttribute : public plManipulatorAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plConeLengthManipulatorAttribute, plManipulatorAttribute);

public:
  plConeLengthManipulatorAttribute();
  plConeLengthManipulatorAttribute(const char* szRadiusProperty);

  const plUntrackedString& GetRadiusProperty() const { return m_sProperty1; }
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plConeAngleManipulatorAttribute : public plManipulatorAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plConeAngleManipulatorAttribute, plManipulatorAttribute);

public:
  plConeAngleManipulatorAttribute();
  plConeAngleManipulatorAttribute(const char* szAngleProperty, float fScale = 1.0f, const char* szRadiusProperty = nullptr);

  const plUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const plUntrackedString& GetRadiusProperty() const { return m_sProperty2; }

  float m_fScale;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plTransformManipulatorAttribute : public plManipulatorAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTransformManipulatorAttribute, plManipulatorAttribute);

public:
  plTransformManipulatorAttribute();
  plTransformManipulatorAttribute(const char* szTranslateProperty, const char* szRotateProperty = nullptr, const char* szScaleProperty = nullptr, const char* szOffsetTranslation = nullptr, const char* szOffsetRotation = nullptr);

  const plUntrackedString& GetTranslateProperty() const { return m_sProperty1; }
  const plUntrackedString& GetRotateProperty() const { return m_sProperty2; }
  const plUntrackedString& GetScaleProperty() const { return m_sProperty3; }
  const plUntrackedString& GetGetOffsetTranslationProperty() const { return m_sProperty4; }
  const plUntrackedString& GetGetOffsetRotationProperty() const { return m_sProperty5; }
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plBoneManipulatorAttribute : public plManipulatorAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plBoneManipulatorAttribute, plManipulatorAttribute);

public:
  plBoneManipulatorAttribute();
  plBoneManipulatorAttribute(const char* szTransformProperty, const char* szBindTo);

  const plUntrackedString& GetTransformProperty() const { return m_sProperty1; }
};

//////////////////////////////////////////////////////////////////////////

struct plVisualizerAnchor
{
  using StorageType = plUInt8;

  enum Enum
  {
    Center = 0,
    PosX = PLASMA_BIT(0),
    NegX = PLASMA_BIT(1),
    PosY = PLASMA_BIT(2),
    NegY = PLASMA_BIT(3),
    PosZ = PLASMA_BIT(4),
    NegZ = PLASMA_BIT(5),

    Default = Center
  };

  struct Bits
  {
    StorageType PosX : 1;
    StorageType NegX : 1;
    StorageType PosY : 1;
    StorageType NegY : 1;
    StorageType PosZ : 1;
    StorageType NegZ : 1;
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_FOUNDATION_DLL, plVisualizerAnchor);

//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plVisualizerAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plVisualizerAttribute, plPropertyAttribute);

public:
  plVisualizerAttribute(const char* szProperty1, const char* szProperty2 = nullptr, const char* szProperty3 = nullptr,
    const char* szProperty4 = nullptr, const char* szProperty5 = nullptr);

  plUntrackedString m_sProperty1;
  plUntrackedString m_sProperty2;
  plUntrackedString m_sProperty3;
  plUntrackedString m_sProperty4;
  plUntrackedString m_sProperty5;
  plBitflags<plVisualizerAnchor> m_Anchor;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plBoxVisualizerAttribute : public plVisualizerAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plBoxVisualizerAttribute, plVisualizerAttribute);

public:
  plBoxVisualizerAttribute();
  plBoxVisualizerAttribute(const char* szSizeProperty, float fSizeScale = 1.0f, const plColor& fixedColor = plColorScheme::LightUI(plColorScheme::Grape), const char* szColorProperty = nullptr, plBitflags<plVisualizerAnchor> anchor = plVisualizerAnchor::Center, plVec3 vOffsetOrScale = plVec3::ZeroVector(), const char* szOffsetProperty = nullptr, const char* szRotationProperty = nullptr);

  const plUntrackedString& GetSizeProperty() const { return m_sProperty1; }
  const plUntrackedString& GetColorProperty() const { return m_sProperty2; }
  const plUntrackedString& GetOffsetProperty() const { return m_sProperty3; }
  const plUntrackedString& GetRotationProperty() const { return m_sProperty4; }

  float m_fSizeScale = 1.0f;
  plColor m_Color;
  plVec3 m_vOffsetOrScale;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plSphereVisualizerAttribute : public plVisualizerAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSphereVisualizerAttribute, plVisualizerAttribute);

public:
  plSphereVisualizerAttribute();
  plSphereVisualizerAttribute(const char* szRadiusProperty, const plColor& fixedColor = plColorScheme::LightUI(plColorScheme::Grape), const char* szColorProperty = nullptr, plBitflags<plVisualizerAnchor> anchor = plVisualizerAnchor::Center, plVec3 vOffsetOrScale = plVec3::ZeroVector(), const char* szOffsetProperty = nullptr);

  const plUntrackedString& GetRadiusProperty() const { return m_sProperty1; }
  const plUntrackedString& GetColorProperty() const { return m_sProperty2; }
  const plUntrackedString& GetOffsetProperty() const { return m_sProperty3; }

  plColor m_Color;
  plVec3 m_vOffsetOrScale;
};


//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plCapsuleVisualizerAttribute : public plVisualizerAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCapsuleVisualizerAttribute, plVisualizerAttribute);

public:
  plCapsuleVisualizerAttribute();
  plCapsuleVisualizerAttribute(const char* szHeightProperty, const char* szRadiusProperty, const plColor& fixedColor = plColorScheme::LightUI(plColorScheme::Grape), const char* szColorProperty = nullptr, plBitflags<plVisualizerAnchor> anchor = plVisualizerAnchor::Center);

  const plUntrackedString& GetHeightProperty() const { return m_sProperty1; }
  const plUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
  const plUntrackedString& GetColorProperty() const { return m_sProperty3; }

  plColor m_Color;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plCylinderVisualizerAttribute : public plVisualizerAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCylinderVisualizerAttribute, plVisualizerAttribute);

public:
  plCylinderVisualizerAttribute();
  plCylinderVisualizerAttribute(plEnum<plBasisAxis> axis, const char* szHeightProperty, const char* szRadiusProperty, const plColor& fixedColor = plColorScheme::LightUI(plColorScheme::Grape), const char* szColorProperty = nullptr, plBitflags<plVisualizerAnchor> anchor = plVisualizerAnchor::Center, plVec3 vOffsetOrScale = plVec3::ZeroVector(), const char* szOffsetProperty = nullptr);
  plCylinderVisualizerAttribute(const char* szAxisProperty, const char* szHeightProperty, const char* szRadiusProperty, const plColor& fixedColor = plColorScheme::LightUI(plColorScheme::Grape), const char* szColorProperty = nullptr, plBitflags<plVisualizerAnchor> anchor = plVisualizerAnchor::Center, plVec3 vOffsetOrScale = plVec3::ZeroVector(), const char* szOffsetProperty = nullptr);

  const plUntrackedString& GetAxisProperty() const { return m_sProperty5; }
  const plUntrackedString& GetHeightProperty() const { return m_sProperty1; }
  const plUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
  const plUntrackedString& GetColorProperty() const { return m_sProperty3; }
  const plUntrackedString& GetOffsetProperty() const { return m_sProperty4; }

  plColor m_Color;
  plVec3 m_vOffsetOrScale;
  plEnum<plBasisAxis> m_Axis;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plDirectionVisualizerAttribute : public plVisualizerAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDirectionVisualizerAttribute, plVisualizerAttribute);

public:
  plDirectionVisualizerAttribute();
  plDirectionVisualizerAttribute(plEnum<plBasisAxis> axis, float fScale, const plColor& fixedColor = plColorScheme::LightUI(plColorScheme::Grape), const char* szColorProperty = nullptr, const char* szLengthProperty = nullptr);
  plDirectionVisualizerAttribute(const char* szAxisProperty, float fScale, const plColor& fixedColor = plColorScheme::LightUI(plColorScheme::Grape), const char* szColorProperty = nullptr, const char* szLengthProperty = nullptr);

  const plUntrackedString& GetColorProperty() const { return m_sProperty1; }
  const plUntrackedString& GetLengthProperty() const { return m_sProperty2; }
  const plUntrackedString& GetAxisProperty() const { return m_sProperty3; }

  plEnum<plBasisAxis> m_Axis;
  plColor m_Color;
  float m_fScale;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plConeVisualizerAttribute : public plVisualizerAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plConeVisualizerAttribute, plVisualizerAttribute);

public:
  plConeVisualizerAttribute();

  /// \brief Attribute to add on an RTTI type to add a cone visualizer for specific properties.
  ///
  /// szRadiusProperty may be nullptr, in which case it is assumed to be 1
  /// fScale will be multiplied with value of szRadiusProperty to determine the size of the cone
  /// szColorProperty may be nullptr. In this case it is ignored and fixedColor is used instead.
  /// fixedColor is ignored if szColorProperty is valid.
  plConeVisualizerAttribute(plEnum<plBasisAxis> axis, const char* szAngleProperty, float fScale, const char* szRadiusProperty, const plColor& fixedColor = plColorScheme::LightUI(plColorScheme::Grape), const char* szColorProperty = nullptr);

  const plUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const plUntrackedString& GetRadiusProperty() const { return m_sProperty2; }
  const plUntrackedString& GetColorProperty() const { return m_sProperty3; }

  plEnum<plBasisAxis> m_Axis;
  plColor m_Color;
  float m_fScale;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_FOUNDATION_DLL plCameraVisualizerAttribute : public plVisualizerAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCameraVisualizerAttribute, plVisualizerAttribute);

public:
  plCameraVisualizerAttribute();

  /// \brief Attribute to add on an RTTI type to add a camera cone visualizer.
  plCameraVisualizerAttribute(const char* szModeProperty, const char* szFovProperty, const char* szOrthoDimProperty, const char* szNearPlaneProperty, const char* szFarPlaneProperty);

  const plUntrackedString& GetModeProperty() const { return m_sProperty1; }
  const plUntrackedString& GetFovProperty() const { return m_sProperty2; }
  const plUntrackedString& GetOrthoDimProperty() const { return m_sProperty3; }
  const plUntrackedString& GetNearPlaneProperty() const { return m_sProperty4; }
  const plUntrackedString& GetFarPlaneProperty() const { return m_sProperty5; }
};

//////////////////////////////////////////////////////////////////////////

// Implementation moved here as it requires plPropertyAttribute to be fully defined.
template <typename Type>
const Type* plRTTI::GetAttributeByType() const
{
  for (const auto* pAttr : m_Attributes)
  {
    if (pAttr->GetDynamicRTTI()->IsDerivedFrom<Type>())
      return static_cast<const Type*>(pAttr);
  }
  if (GetParentType() != nullptr)
    return GetParentType()->GetAttributeByType<Type>();
  else
    return nullptr;
}

template <typename Type>
const Type* plAbstractProperty::GetAttributeByType() const
{
  for (const auto* pAttr : m_Attributes)
  {
    if (pAttr->GetDynamicRTTI()->IsDerivedFrom<Type>())
      return static_cast<const Type*>(pAttr);
  }
  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

/// \brief A property attribute that specifies the max size of an array. If it is reached, no further elemets are allowed to be added.
class PLASMA_FOUNDATION_DLL plMaxArraySizeAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMaxArraySizeAttribute, plPropertyAttribute);

public:
  plMaxArraySizeAttribute() = default;
  plMaxArraySizeAttribute(plUInt32 uiMaxSize) { m_uiMaxSize = uiMaxSize; }

  const plUInt32& GetMaxSize() const { return m_uiMaxSize; }

private:
  plUInt32 m_uiMaxSize = 0;
};

//////////////////////////////////////////////////////////////////////////

/// \brief If this attribute is set, the UI is encouraged to prevent the user from creating duplicates of the same thing.
///
/// For arrays of objects this means that multiple objects of the same type are not allowed.
class PLASMA_FOUNDATION_DLL plPreventDuplicatesAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPreventDuplicatesAttribute, plPropertyAttribute);

public:
  plPreventDuplicatesAttribute() = default;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Attribute for types that should not be exposed to the scripting framework
class PLASMA_FOUNDATION_DLL plExcludeFromScript : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plExcludeFromScript, plPropertyAttribute);
};

/// \brief Attribute to mark a function up to be exposed to the scripting system. Arguments specify the names of the function parameters.
class PLASMA_FOUNDATION_DLL plScriptableFunctionAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plScriptableFunctionAttribute, plPropertyAttribute);

  enum ArgType : plUInt8
  {
    In,
    Out,
    Inout
  };

  plScriptableFunctionAttribute(ArgType argType1 = In, const char* szArg1 = nullptr, ArgType argType2 = In, const char* szArg2 = nullptr,
    ArgType argType3 = In, const char* szArg3 = nullptr, ArgType argType4 = In, const char* szArg4 = nullptr, ArgType argType5 = In,
    const char* szArg5 = nullptr, ArgType argType6 = In, const char* szArg6 = nullptr);

  const char* GetArgumentName(plUInt32 uiIndex) const { return m_ArgNames[uiIndex]; }

  ArgType GetArgumentType(plUInt32 uiIndex) const { return static_cast<ArgType>(m_ArgTypes[uiIndex]); }

private:
  plHybridArray<plUntrackedString, 6> m_ArgNames;
  plHybridArray<plUInt8, 6> m_ArgTypes;
};

/// \brief Wrapper Attribute to add an attribute to a function argument
class PLASMA_FOUNDATION_DLL plFunctionArgumentAttributes : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plFunctionArgumentAttributes, plPropertyAttribute);

  plFunctionArgumentAttributes() = default;
  plFunctionArgumentAttributes(plUInt32 uiArgIndex, const plPropertyAttribute* pAttribute1, const plPropertyAttribute* pAttribute2 = nullptr, const plPropertyAttribute* pAttribute3 = nullptr, const plPropertyAttribute* pAttribute4 = nullptr);

  plUInt32 GetArgumentIndex() const { return m_uiArgIndex; }
  plArrayPtr<const plPropertyAttribute* const> GetArgumentAttributes() const { return m_ArgAttributes; }

private:
  plUInt32 m_uiArgIndex = 0;
  plHybridArray<const plPropertyAttribute*, 4> m_ArgAttributes;
};

/// \brief Used to mark an array or (unsigned)int property as source for dynamic pin generation on nodes
class PLASMA_FOUNDATION_DLL plDynamicPinAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDynamicPinAttribute, plPropertyAttribute);

public:
  plDynamicPinAttribute() = default;
  plDynamicPinAttribute(const char* szProperty);

  const plUntrackedString& GetProperty() const { return m_sProperty; }

private:
  plUntrackedString m_sProperty;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Used to mark that a component provides functionality that is executed with a long operation in the editor.
///
/// \a szOpTypeName must be the class name of a class derived from plLongOpProxy.
/// Once a component is added to a scene with this attribute, the named long op will appear in the UI and can be executed.
///
/// The automatic registration is done by plLongOpsAdapter
class PLASMA_FOUNDATION_DLL plLongOpAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLongOpAttribute, plPropertyAttribute);

public:
  plLongOpAttribute() = default;
  plLongOpAttribute(const char* szOpTypeName)
    : m_sOpTypeName(szOpTypeName)
  {
  }

  plUntrackedString m_sOpTypeName;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A property attribute that indicates that the string property is actually a game object reference.
class PLASMA_FOUNDATION_DLL plGameObjectReferenceAttribute : public plTypeWidgetAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameObjectReferenceAttribute, plTypeWidgetAttribute);

public:
  plGameObjectReferenceAttribute() = default;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A property attribute that indicates the property should be left aligned with the next one on its right
/// \note Do not use on a single property or the last property of a list, as it would cause bad alignment
class PLASMA_FOUNDATION_DLL plGroupNextAttribute : public plTypeWidgetAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGroupNextAttribute, plTypeWidgetAttribute);

public:
  plGroupNextAttribute() = default;
};
