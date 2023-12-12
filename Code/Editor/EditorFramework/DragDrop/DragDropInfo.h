#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class QMimeData;
class QDataStream;
class plDocumentObject;
class plQtDocumentTreeModelAdapter;

/// \brief This type is used to provide plDragDropHandler instances with all the important information for a drag & drop target
///
/// It is a reflected class such that one can derive and extend it, if necessary.
/// DragDrop handlers can then inspect whether it is a known extended type and cast to the type to get access to additional information.
class PLASMA_EDITORFRAMEWORK_DLL plDragDropInfo : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDragDropInfo, plReflectedClass);

public:
  plDragDropInfo();

  const QMimeData* m_pMimeData;

  /// A string identifying into what context the object is dropped, e.g. "viewport" or "scenetree" etc.
  plString m_sTargetContext;

  /// The plDocument GUID
  plUuid m_TargetDocument;

  /// GUID of the plDocumentObject that is at the dropped position. May be invalid. Can be used to attach as a child, to modify the object itself or
  /// can be ignored.
  plUuid m_TargetObject;

  /// GUID of the plDocumentObject that is the more specific component (of m_TargetObject) that was dragged on. May be invalid.
  plUuid m_TargetComponent;

  /// World space position where the object is dropped. May be NaN.
  plVec3 m_vDropPosition;

  /// World space normal at the point where the object is dropped. May be NaN.
  plVec3 m_vDropNormal;

  /// Some kind of index / ID for the object that is at the drop location. For meshes this is the material index.
  plInt32 m_iTargetObjectSubID;

  /// If dropped on a scene tree, this may say as which child the object is supposed to be inserted. -1 if invalid (ie. append)
  plInt32 m_iTargetObjectInsertChildIndex;

  /// If dropped on a scene tree, this is the adapter for the target object.
  const plQtDocumentTreeModelAdapter* m_pAdapter = nullptr;

  bool m_bShiftKeyDown;
  bool m_bCtrlKeyDown;
};


/// \brief After an plDragDropHandler has been chosen to handle an operation, it is queried once to fill out an instance of this type (or an extended
/// derived type) to enable configuring how plDragDropInfo is computed by the target.
class PLASMA_EDITORFRAMEWORK_DLL plDragDropConfig : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDragDropConfig, plReflectedClass);

public:
  plDragDropConfig();

  /// Whether the currently selected objects (ie the dragged objects) should be considered for picking or not. Default is disabled.
  bool m_bPickSelectedObjects;
};

/// \brief Helper operator to retrieve the "application/PlasmaEditor.ObjectSelection" mime data from a plDragDropInfo::m_pMimeData.
/// \code{.cpp}
///   plHybridArray<const plDocumentObject*, 32> Dragged;
///   QByteArray encodedData = m_pMimeData->data("application/PlasmaEditor.ObjectSelection");
///   QDataStream stream(&encodedData, QIODevice::ReadOnly);
///   stream >> Dragged;
/// \endcode
PLASMA_EDITORFRAMEWORK_DLL void operator>>(QDataStream& stream, plDynamicArray<plDocumentObject*>& rhs);
