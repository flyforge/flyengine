#pragma once

#include <Core/World/World.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Types/TagSet.h>

/// \brief Stores an entire plWorld in a stream.
///
/// Used for exporting a world in binary form either as a level or as a prefab (though there is no
/// difference).
/// Can be used for saving a game, if the exact state of the world shall be stored (e.g. like in an FPS).
class PL_CORE_DLL plWorldWriter
{
public:
  /// \brief Writes all content in \a world to \a stream.
  ///
  /// All game objects with tags that overlap with \a pExclude will be ignored.
  void WriteWorld(plStreamWriter& inout_stream, plWorld& ref_world, const plTagSet* pExclude = nullptr);

  /// \brief Only writes the given root objects and all their children to the stream.
  void WriteObjects(plStreamWriter& inout_stream, const plDeque<const plGameObject*>& rootObjects);

  /// \brief Only writes the given root objects and all their children to the stream.
  void WriteObjects(plStreamWriter& inout_stream, plArrayPtr<const plGameObject*> rootObjects);

  /// \brief Writes the given game object handle to the stream.
  ///
  /// \note If the handle belongs to an object that is not part of the serialized scene, e.g. an object
  /// that was excluded by a tag, this function will assert.
  void WriteGameObjectHandle(const plGameObjectHandle& hObject);

  /// \brief Writes the given component handle to the stream.
  ///
  /// \note If the handle belongs to a component that is not part of the serialized scene, e.g. an object
  /// that was excluded by a tag, this function will assert.
  void WriteComponentHandle(const plComponentHandle& hComponent);

  /// \brief Accesses the stream to which data is written. Use this in component serialization functions
  /// to write data to the stream.
  plStreamWriter& GetStream() const { return *m_pStream; }

  /// \brief Returns an array containing all game object pointers that were written to the stream as root objects
  const plDeque<const plGameObject*>& GetAllWrittenRootObjects() const { return m_AllRootObjects; }

  /// \brief Returns an array containing all game object pointers that were written to the stream as child objects
  const plDeque<const plGameObject*>& GetAllWrittenChildObjects() const { return m_AllChildObjects; }

private:
  void Clear();
  plResult WriteToStream();
  void AssignGameObjectIndices();
  void AssignComponentHandleIndices(const plMap<plString, const plRTTI*>& sortedTypes);
  void IncludeAllComponentBaseTypes();
  void IncludeAllComponentBaseTypes(const plRTTI* pRtti);
  void Traverse(plGameObject* pObject);

  plVisitorExecution::Enum ObjectTraverser(plGameObject* pObject);
  void WriteGameObject(const plGameObject* pObject);
  void WriteComponentTypeInfo(const plRTTI* pRtti);
  void WriteComponentCreationData(const plDeque<const plComponent*>& components);
  void WriteComponentSerializationData(const plDeque<const plComponent*>& components);

  plStreamWriter* m_pStream = nullptr;
  const plTagSet* m_pExclude = nullptr;

  plDeque<const plGameObject*> m_AllRootObjects;
  plDeque<const plGameObject*> m_AllChildObjects;
  plMap<plGameObjectHandle, plUInt32> m_WrittenGameObjectHandles;

  struct Components
  {
    plUInt16 m_uiSerializedTypeIndex = 0;
    plDeque<const plComponent*> m_Components;
    plMap<plComponentHandle, plUInt32> m_HandleToIndex;
  };

  plHashTable<const plRTTI*, Components> m_AllComponents;
};
