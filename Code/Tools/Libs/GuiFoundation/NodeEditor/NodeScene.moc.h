#pragma once

#include <Foundation/Containers/Map.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class plQtNode;
class plQtPin;
class plQtConnection;
struct plSelectionManagerEvent;

class PLASMA_GUIFOUNDATION_DLL plQtNodeScene : public QGraphicsScene
{
  Q_OBJECT
public:
  enum Type
  {
    Node = QGraphicsItem::UserType + 1,
    Pin,
    Connection
  };

  explicit plQtNodeScene(QObject* pParent = nullptr);
  ~plQtNodeScene();

  virtual void InitScene(const plDocumentNodeManager* pManager);
  const plDocumentNodeManager* GetDocumentNodeManager() const;
  const plDocument* GetDocument() const;

  static plRttiMappedObjectFactory<plQtNode>& GetNodeFactory();
  static plRttiMappedObjectFactory<plQtPin>& GetPinFactory();
  static plRttiMappedObjectFactory<plQtConnection>& GetConnectionFactory();
  static plVec2 GetLastMouseInteractionPos() { return s_vLastMouseInteraction; }

  struct ConnectionStyle
  {
    using StorageType = plUInt32;

    enum Enum
    {
      BezierCurve,
      StraightLine,
      Subway,

      Default = Subway
    };
  };

  void SetConnectionStyle(plEnum<ConnectionStyle> style);
  plEnum<ConnectionStyle> GetConnectionStyle() const { return m_ConnectionStyle; }

  struct ConnectionDecorationFlags
  {
    using StorageType = plUInt32;

    enum Enum
    {
      DirectionArrows = PLASMA_BIT(0), ///< Draw an arrow to indicate the connection's direction. Only works with straight lines atm.

      Default = 0
    };

    struct Bits
    {
      StorageType DirectionArrows : 1;
    };
  };

  void SetConnectionDecorationFlags(plBitflags<ConnectionDecorationFlags> flags);
  plBitflags<ConnectionDecorationFlags> GetConnectionDecorationFlags() const { return m_ConnectionDecorationFlags; }

protected:
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent) override;
  virtual void keyPressEvent(QKeyEvent* event) override;

private:
  void Clear();
  void CreateQtNode(const plDocumentObject* pObject);
  void DeleteQtNode(const plDocumentObject* pObject);
  void CreateQtConnection(const plDocumentObject* pObject);
  void DeleteQtConnection(const plDocumentObject* pObject);
  void RecreateQtPins(const plDocumentObject* pObject);
  void CreateNodeObject(const plRTTI* pRtti);
  void NodeEventsHandler(const plDocumentNodeManagerEvent& e);
  void PropertyEventsHandler(const plDocumentObjectPropertyEvent& e);
  void SelectionEventsHandler(const plSelectionManagerEvent& e);
  void GetSelectedNodes(plDeque<plQtNode*>& selection) const;
  void MarkupConnectablePins(plQtPin* pSourcePin);
  void ResetConnectablePinMarkup();
  void OpenSearchMenu(QPoint screenPos);

protected:
  virtual plStatus RemoveNode(plQtNode* pNode);
  virtual void RemoveSelectedNodesAction();
  virtual void ConnectPinsAction(const plPin& sourcePin, const plPin& targetPin);
  virtual void DisconnectPinsAction(plQtConnection* pConnection);
  virtual void DisconnectPinsAction(plQtPin* pPin);

private Q_SLOTS:
  void OnMenuItemTriggered(const QString& sName, const QVariant& variant);
  void OnSelectionChanged();

private:
  static plRttiMappedObjectFactory<plQtNode> s_NodeFactory;
  static plRttiMappedObjectFactory<plQtPin> s_PinFactory;
  static plRttiMappedObjectFactory<plQtConnection> s_ConnectionFactory;

protected:
  const plDocumentNodeManager* m_pManager = nullptr;

  plMap<const plDocumentObject*, plQtNode*> m_Nodes;
  plMap<const plDocumentObject*, plQtConnection*> m_Connections;

private:
  bool m_bIgnoreSelectionChange = false;
  plQtPin* m_pStartPin = nullptr;
  plQtConnection* m_pTempConnection = nullptr;
  plQtNode* m_pTempNode = nullptr;
  plDeque<const plDocumentObject*> m_Selection;
  plVec2 m_vMousePos = plVec2::MakeZero();
  QString m_sContextMenuSearchText;
  plDynamicArray<const plQtPin*> m_ConnectablePins;
  plEnum<ConnectionStyle> m_ConnectionStyle;
  plBitflags<ConnectionDecorationFlags> m_ConnectionDecorationFlags;

  static plVec2 s_vLastMouseInteraction;
};

PLASMA_DECLARE_FLAGS_OPERATORS(plQtNodeScene::ConnectionDecorationFlags);
