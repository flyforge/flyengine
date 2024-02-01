#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Strings/HashedString.h>

struct plSpatialData
{
  struct Flags
  {
    using StorageType = plUInt8;

    enum Enum
    {
      None = 0,
      FrequentChanges = PL_BIT(0), ///< Indicates that objects in this category change their bounds frequently. Spatial System implementations can use that as hint for internal optimizations.

      Default = None
    };

    struct Bits
    {
      StorageType FrequentUpdates : 1;
    };
  };

  struct Category
  {
    PL_ALWAYS_INLINE Category()
      : m_uiValue(plSmallInvalidIndex)
    {
    }

    PL_ALWAYS_INLINE explicit Category(plUInt16 uiValue)
      : m_uiValue(uiValue)
    {
    }

    PL_ALWAYS_INLINE bool operator==(const Category& other) const { return m_uiValue == other.m_uiValue; }
    PL_ALWAYS_INLINE bool operator!=(const Category& other) const { return m_uiValue != other.m_uiValue; }

    plUInt16 m_uiValue;

    PL_ALWAYS_INLINE plUInt32 GetBitmask() const { return m_uiValue != plSmallInvalidIndex ? static_cast<plUInt32>(PL_BIT(m_uiValue)) : 0; }
  };

  /// \brief Registers a spatial data category under the given name.
  ///
  /// If the same category was already registered before, it returns that instead.
  /// Asserts that there are no more than 32 unique categories.
  PL_CORE_DLL static Category RegisterCategory(plStringView sCategoryName, const plBitflags<Flags>& flags);

  /// \brief Returns either an existing category with the given name or plInvalidSpatialDataCategory.
  PL_CORE_DLL static Category FindCategory(plStringView sCategoryName);

  /// \brief Returns the name of the given category.
  PL_CORE_DLL static const plHashedString& GetCategoryName(Category category);

  /// \brief Returns the flags for the given category.
  PL_CORE_DLL static const plBitflags<Flags>& GetCategoryFlags(Category category);

private:
  struct CategoryData
  {
    plHashedString m_sName;
    plBitflags<Flags> m_Flags;
  };

  static plHybridArray<plSpatialData::CategoryData, 32>& GetCategoryData();
};

struct PL_CORE_DLL plDefaultSpatialDataCategories
{
  static plSpatialData::Category RenderStatic;
  static plSpatialData::Category RenderDynamic;
  static plSpatialData::Category OcclusionStatic;
  static plSpatialData::Category OcclusionDynamic;
};

/// \brief When an object is 'seen' by a view and thus tagged as 'visible', this enum describes what kind of observer triggered this.
///
/// This is used to determine how important certain updates, such as animations, are to execute.
/// E.g. when a 'shadow view' or 'reflection view' is the only thing that observes an object, animations / particle effects and so on,
/// can be updated less frequently.
enum class plVisibilityState : plUInt8
{
  Invisible = 0, ///< The object isn't visible to any view.
  Indirect = 1,  ///< The object is seen by a view that only indirectly makes the object visible (shadow / reflection / render target).
  Direct = 2,    ///< The object is seen directly by a main view and therefore it needs to be updated at maximum frequency.
};

#define plInvalidSpatialDataCategory plSpatialData::Category()
