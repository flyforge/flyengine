
#pragma once

#include <Foundation/Strings/HashedString.h>

using plTagSetBlockStorage = plUInt64;

/// \brief The tag class stores the necessary lookup information for a single tag which can be used in conjunction with the tag set.
///
/// A tag is the storage for a small amount of lookup information for a single tag. Instances
/// of plTag can be used in checks with the tag set. Note that fetching information for the tag needs to access
/// the global tag registry which involves a mutex lock. It is thus
/// recommended to fetch tag instances early and reuse them for the actual tests and to avoid querying the tag registry
/// all the time (e.g. due to tag instances being kept on the stack).
class PLASMA_FOUNDATION_DLL plTag
{
public:
  PLASMA_ALWAYS_INLINE plTag();

  PLASMA_ALWAYS_INLINE bool operator==(const plTag& rhs) const; // [tested]

  PLASMA_ALWAYS_INLINE bool operator!=(const plTag& rhs) const; // [tested]

  PLASMA_ALWAYS_INLINE bool operator<(const plTag& rhs) const;

  PLASMA_ALWAYS_INLINE const plString& GetTagString() const; // [tested]

  PLASMA_ALWAYS_INLINE bool IsValid() const; // [tested]

private:
  template <typename BlockStorageAllocator>
  friend class plTagSetTemplate;
  friend class plTagRegistry;

  plHashedString m_sTagString;

  plUInt32 m_uiBitIndex = 0xFFFFFFFEu;
  plUInt32 m_uiBlockIndex = 0xFFFFFFFEu;
};

#include <Foundation/Types/TagSet.h>

#include <Foundation/Types/Implementation/Tag_inl.h>
