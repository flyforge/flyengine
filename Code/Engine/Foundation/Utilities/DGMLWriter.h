#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Strings/String.h>

class plFormatString;

/// \brief This class encapsulates building a DGML compatible graph.
class PL_FOUNDATION_DLL plDGMLGraph
{
public:
  enum class Direction : plUInt8
  {
    TopToBottom,
    BottomToTop,
    LeftToRight,
    RightToLeft
  };

  enum class Layout : plUInt8
  {
    Free,
    Tree,
    DependencyMatrix
  };

  enum class NodeShape : plUInt8
  {
    None,
    Rectangle,
    RoundedRectangle,
    Button
  };

  enum class GroupType : plUInt8
  {
    None,
    Expanded,
    Collapsed,
  };

  using NodeId = plUInt32;
  using PropertyId = plUInt32;
  using ConnectionId = plUInt32;

  struct NodeDesc
  {
    plColor m_Color = plColor::White;
    NodeShape m_Shape = NodeShape::Rectangle;
  };

  /// \brief Constructor for the graph.
  plDGMLGraph(Direction graphDirection = Direction::LeftToRight, Layout graphLayout = Layout::Tree);

  /// \brief Adds a node to the graph.
  /// Adds a node to the graph and returns the node id which can be used to reference the node later to add connections etc.
  NodeId AddNode(plStringView sTitle, const NodeDesc* pDesc = nullptr);

  /// \brief Adds a DGML node that can act as a group for other nodes
  NodeId AddGroup(plStringView sTitle, GroupType type, const NodeDesc* pDesc = nullptr);

  /// \brief Inserts a node into an existing group node.
  void AddNodeToGroup(NodeId node, NodeId group);

  /// \brief Adds a directed connection to the graph (an arrow pointing from source to target node).
  ConnectionId AddConnection(NodeId source, NodeId target, plStringView sLabel = {});

  /// \brief Adds a property type. All properties currently use the data type 'string'
  PropertyId AddPropertyType(plStringView sName);

  /// \brief Adds a property of the specified type with the given value to a node
  void AddNodeProperty(NodeId node, PropertyId property, const plFormatString& fmt);

protected:
  friend class plDGMLGraphWriter;

  struct Connection
  {
    NodeId m_Source;
    NodeId m_Target;
    plString m_sLabel;
  };

  struct PropertyType
  {
    plString m_Name;
  };

  struct PropertyValue
  {
    PropertyId m_PropertyId;
    plString m_sValue;
  };

  struct Node
  {
    plString m_Title;
    GroupType m_GroupType = GroupType::None;
    NodeId m_ParentGroup = 0xFFFFFFFF;
    NodeDesc m_Desc;
    plDynamicArray<PropertyValue> m_Properties;
  };

  plHybridArray<Node, 16> m_Nodes;

  plHybridArray<Connection, 32> m_Connections;

  plHybridArray<PropertyType, 16> m_PropertyTypes;

  Direction m_Direction;

  Layout m_Layout;
};

/// \brief This class encapsulates the output of DGML compatible graphs to files and streams.
class PL_FOUNDATION_DLL plDGMLGraphWriter
{
public:
  /// \brief Helper method to write the graph to a file.
  static plResult WriteGraphToFile(plStringView sFileName, const plDGMLGraph& graph);

  /// \brief Writes the graph as a DGML formatted document to the given string builder.
  static plResult WriteGraphToString(plStringBuilder& ref_sStringBuilder, const plDGMLGraph& graph);
};
