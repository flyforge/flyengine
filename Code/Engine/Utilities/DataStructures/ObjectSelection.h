#pragma once

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Containers/Deque.h>
#include <Utilities/UtilitiesDLL.h>

/// \brief Stores a list of game objects as a 'selection'. Provides some common convenience functions for working with selections.
class PLASMA_UTILITIES_DLL plObjectSelection
{
public:
  plObjectSelection();

  /// \brief The plWorld in which the game objects are stored.
  void SetWorld(plWorld* pWorld);

  /// \brief Returns the plWorld in which the game objects live.
  const plWorld* GetWorld() const { return m_pWorld; }

  /// \brief Clears the selection.
  void Clear() { m_Objects.Clear(); }

  /// \brief Iterates over all objects and removes the ones that have been destroyed from the selection.
  void RemoveDeadObjects();

  /// \brief Adds the given object to the selection, unless it is not valid anymore. Objects can be added multiple times.
  void AddObject(plGameObjectHandle hObject, bool bDontAddTwice = true);

  /// \brief Removes the first occurrence of the given object from the selection. Returns false if the object did not exist in the
  /// selection.
  bool RemoveObject(plGameObjectHandle hObject);

  /// \brief Removes the object from the selection if it exists already, otherwise adds it.
  void ToggleSelection(plGameObjectHandle hObject);

  /// \brief Returns the number of objects in the selection.
  plUInt32 GetCount() const { return m_Objects.GetCount(); }

  /// \brief Returns the n-th object in the selection.
  plGameObjectHandle GetObject(plUInt32 index) const { return m_Objects[index]; }

private:
  plWorld* m_pWorld;
  plDeque<plGameObjectHandle> m_Objects;
};
