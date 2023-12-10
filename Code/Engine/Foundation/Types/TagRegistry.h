
#pragma once

class plHashedString;
class plTempHashedString;
class plTag;
class plStreamWriter;
class plStreamReader;

#include <Foundation/Containers/Map.h>
#include <Foundation/Threading/Mutex.h>

/// \brief The tag registry for tags in tag sets.
///
/// Normal usage of the tag registry is to get the global tag registry instance via plTagRegistry::GetGlobalRegistry()
/// and to use this instance to register and get tags.
/// Certain special cases (e.g. tests) may actually need their own instance of the tag registry.
/// Note however that tags which were registered with one registry shouldn't be used with tag sets filled
/// with tags from another registry since there may be conflicting tag assignments.
/// The tag registry registration and tag retrieval functions are thread safe due to a mutex.
class PLASMA_FOUNDATION_DLL plTagRegistry
{
public:
  plTagRegistry();

  static plTagRegistry& GetGlobalRegistry();

  /// \brief Ensures the tag with the given name exists and returns a pointer to it.
  const plTag& RegisterTag(plStringView sTagString); // [tested]

  /// \brief Ensures the tag with the given name exists and returns a pointer to it.
  const plTag& RegisterTag(const plHashedString& sTagString); // [tested]

  /// \brief Searches for a tag with the given name and returns a pointer to it
  const plTag* GetTagByName(const plTempHashedString& sTagString) const; // [tested]

  /// \brief Searches for a tag with the given murmur hash. This function is only for backwards compatibility.
  const plTag* GetTagByMurmurHash(plUInt32 uiMurmurHash) const;

  /// \brief Returns the tag with the given index.
  const plTag* GetTagByIndex(plUInt32 uiIndex) const;

  /// \brief Returns the number of registered tags.
  plUInt32 GetNumTags() const;

  /// \brief Loads the saved state and integrates it into this registry. Does not discard previously registered tag information. This function is only
  /// for backwards compatibility.
  plResult Load(plStreamReader& inout_stream);

protected:
  mutable plMutex m_TagRegistryMutex;

  plMap<plTempHashedString, plTag> m_RegisteredTags;
  plDeque<plTag*> m_TagsByIndex;
};
