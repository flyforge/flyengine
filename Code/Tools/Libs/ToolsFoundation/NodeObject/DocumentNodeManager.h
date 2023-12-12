#pragma once

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plPin;
class plConnection;

struct PLASMA_TOOLSFOUNDATION_DLL plDocumentNodeManagerEvent
{
  enum class Type
  {
    NodeMoved,
    AfterPinsConnected,
    BeforePinsDisonnected,
    BeforePinsChanged,
    AfterPinsChanged,
    BeforeNodeAdded,
    AfterNodeAdded,
    BeforeNodeRemoved,
    AfterNodeRemoved,
  };

  plDocumentNodeManagerEvent(Type eventType, const plDocumentObject* pObject = nullptr)
    : m_EventType(eventType)
    , m_pObject(pObject)
  {
  }

  Type m_EventType;
  const plDocumentObject* m_pObject;
};

class plConnection
{
public:
  const plPin& GetSourcePin() const { return m_SourcePin; }
  const plPin& GetTargetPin() const { return m_TargetPin; }
  const plDocumentObject* GetParent() const { return m_pParent; }

private:
  friend class plDocumentNodeManager;

  plConnection(const plPin& sourcePin, const plPin& targetPin, const plDocumentObject* pParent)
    : m_SourcePin(sourcePin)
    , m_TargetPin(targetPin)
    , m_pParent(pParent)
  {
  }

  const plPin& m_SourcePin;
  const plPin& m_TargetPin;
  const plDocumentObject* m_pParent = nullptr;
};

class PLASMA_TOOLSFOUNDATION_DLL plPin : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPin, plReflectedClass);

public:
  enum class Type
  {
    Input,
    Output
  };

  enum class Shape
  {
    Circle,
    Rect,
    RoundRect,
    Arrow,
    Default = Circle
  };

  plPin(Type type, plStringView sName, const plColorGammaUB& color, const plDocumentObject* pObject)
    : m_Type(type)
    , m_Color(color)
    , m_sName(sName)
    , m_pParent(pObject)
  {
  }

  Shape m_Shape = Shape::Default;

  Type GetType() const { return m_Type; }
  const char* GetName() const { return m_sName; }
  const plColorGammaUB& GetColor() const { return m_Color; }
  const plDocumentObject* GetParent() const { return m_pParent; }

private:
  friend class plDocumentNodeManager;

  Type m_Type;
  plColorGammaUB m_Color;
  plString m_sName;
  const plDocumentObject* m_pParent = nullptr;
};

class PLASMA_TOOLSFOUNDATION_DLL plDocumentNodeManager : public plDocumentObjectManager
{
public:
  plEvent<const plDocumentNodeManagerEvent&> m_NodeEvents;

  plDocumentNodeManager();
  virtual ~plDocumentNodeManager();

  virtual const plRTTI* GetConnectionType() const;

  plVec2 GetNodePos(const plDocumentObject* pObject) const;
  const plConnection& GetConnection(const plDocumentObject* pObject) const;

  const plPin* GetInputPinByName(const plDocumentObject* pObject, const char* szName) const;
  const plPin* GetOutputPinByName(const plDocumentObject* pObject, const char* szName) const;
  plArrayPtr<const plUniquePtr<const plPin>> GetInputPins(const plDocumentObject* pObject) const;
  plArrayPtr<const plUniquePtr<const plPin>> GetOutputPins(const plDocumentObject* pObject) const;

  enum class CanConnectResult
  {
    ConnectNever, ///< Pins can't be connected
    Connect1to1,  ///< Output pin can have 1 outgoing connection, Input pin can have 1 incoming connection
    Connect1toN,  ///< Output pin can have 1 outgoing connection, Input pin can have N incoming connections
    ConnectNto1,  ///< Output pin can have N outgoing connections, Input pin can have 1 incoming connection
    ConnectNtoN,  ///< Output pin can have N outgoing connections, Input pin can have N incoming connections
  };

  bool IsNode(const plDocumentObject* pObject) const;
  bool IsConnection(const plDocumentObject* pObject) const;
  bool IsDynamicPinProperty(const plDocumentObject* pObject, const plAbstractProperty* pProp) const;

  plArrayPtr<const plConnection* const> GetConnections(const plPin& pin) const;
  bool HasConnections(const plPin& pin) const;
  bool IsConnected(const plPin& source, const plPin& target) const;
  plStatus CanConnect(const plRTTI* pObjectType, const plPin& source, const plPin& target, CanConnectResult& result) const;
  plStatus CanDisconnect(const plConnection* pConnection) const;
  plStatus CanDisconnect(const plDocumentObject* pObject) const;
  plStatus CanMoveNode(const plDocumentObject* pObject, const plVec2& vPos) const;

  void Connect(const plDocumentObject* pObject, const plPin& source, const plPin& target);
  void Disconnect(const plDocumentObject* pObject);
  void MoveNode(const plDocumentObject* pObject, const plVec2& vPos);

  void AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const;
  void RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable);

  void GetMetaDataHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const;
  bool CopySelectedObjects(plAbstractObjectGraph& out_objectGraph) const;
  bool PasteObjects(const plArrayPtr<plDocument::PasteInfo>& info, const plAbstractObjectGraph& objectGraph, const plVec2& pickedPosition, bool bAllowPickedPosition);

protected:
  /// \brief Tests whether pTarget can be reached from pSource by following the pin connections
  bool CanReachNode(const plDocumentObject* pSource, const plDocumentObject* pTarget, plSet<const plDocumentObject*>& Visited) const;

  /// \brief Returns true if adding a connection between the two pins would create a circular graph
  bool WouldConnectionCreateCircle(const plPin& source, const plPin& target) const;

  void GetDynamicPinNames(const plDocumentObject* pObject, const char* szPropertyName, plStringView sPinName, plDynamicArray<plString>& out_Names) const;
  virtual bool TryRecreatePins(const plDocumentObject* pObject);

  struct NodeInternal
  {
    plVec2 m_vPos = plVec2::ZeroVector();
    plHybridArray<plUniquePtr<plPin>, 6> m_Inputs;
    plHybridArray<plUniquePtr<plPin>, 6> m_Outputs;
  };

private:
  virtual bool InternalIsNode(const plDocumentObject* pObject) const;
  virtual bool InternalIsConnection(const plDocumentObject* pObject) const;
  virtual bool InternalIsDynamicPinProperty(const plDocumentObject* pObject, const plAbstractProperty* pProp) const { return false; }
  virtual plStatus InternalCanConnect(const plPin& source, const plPin& target, CanConnectResult& out_Result) const;
  virtual plStatus InternalCanDisconnect(const plPin& source, const plPin& target) const { return plStatus(PLASMA_SUCCESS); }
  virtual plStatus InternalCanMoveNode(const plDocumentObject* pObject, const plVec2& vPos) const { return plStatus(PLASMA_SUCCESS); }
  virtual void InternalCreatePins(const plDocumentObject* pObject, NodeInternal& node) = 0;

  void ObjectHandler(const plDocumentObjectEvent& e);
  void StructureEventHandler(const plDocumentObjectStructureEvent& e);
  void PropertyEventsHandler(const plDocumentObjectPropertyEvent& e);

  void RestoreOldMetaDataAfterLoading(const plAbstractObjectGraph& graph, const plAbstractObjectNode::Property& connectionsProperty, const plDocumentObject* pSourceObject);

private:
  plHashTable<plUuid, NodeInternal> m_ObjectToNode;
  plHashTable<plUuid, plUniquePtr<plConnection>> m_ObjectToConnection;
  plMap<const plPin*, plHybridArray<const plConnection*, 6>> m_Connections;
};
