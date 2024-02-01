#include <Utilities/UtilitiesPCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <Utilities/DGML/DGMLCreator.h>

void plDGMLGraphCreator::FillGraphFromWorld(plWorld* pWorld, plDGMLGraph& ref_graph)
{
  if (!pWorld)
  {
    plLog::Warning("plDGMLGraphCreator::FillGraphFromWorld() called with null world!");
    return;
  }


  struct GraphVisitor
  {
    GraphVisitor(plDGMLGraph& ref_graph)
      : m_Graph(ref_graph)
    {
      plDGMLGraph::NodeDesc nd;
      nd.m_Color = plColor::DarkRed;
      nd.m_Shape = plDGMLGraph::NodeShape::Button;
      m_WorldNodeId = ref_graph.AddNode("World", &nd);
    }

    plVisitorExecution::Enum Visit(plGameObject* pObject)
    {
      plStringBuilder name;
      name.SetFormat("GameObject: \"{0}\"", pObject->GetName().IsEmpty() ? "<Unnamed>" : pObject->GetName());

      // Create node for game object
      plDGMLGraph::NodeDesc gameobjectND;
      gameobjectND.m_Color = plColor::CornflowerBlue;
      gameobjectND.m_Shape = plDGMLGraph::NodeShape::Rectangle;
      auto gameObjectNodeId = m_Graph.AddNode(name.GetData(), &gameobjectND);

      m_VisitedObjects.Insert(pObject, gameObjectNodeId);

      // Add connection to parent if existent
      if (const plGameObject* parent = pObject->GetParent())
      {
        auto it = m_VisitedObjects.Find(parent);

        if (it.IsValid())
        {
          m_Graph.AddConnection(gameObjectNodeId, it.Value());
        }
      }
      else
      {
        // No parent -> connect to world
        m_Graph.AddConnection(gameObjectNodeId, m_WorldNodeId);
      }

      // Add components
      for (auto component : pObject->GetComponents())
      {
        auto sComponentName = component->GetDynamicRTTI()->GetTypeName();

        plDGMLGraph::NodeDesc componentND;
        componentND.m_Color = plColor::LimeGreen;
        componentND.m_Shape = plDGMLGraph::NodeShape::RoundedRectangle;
        auto componentNodeId = m_Graph.AddNode(sComponentName, &componentND);

        // And add the link to the game object

        m_Graph.AddConnection(componentNodeId, gameObjectNodeId);
      }

      return plVisitorExecution::Continue;
    }

    plDGMLGraph& m_Graph;

    plDGMLGraph::NodeId m_WorldNodeId;
    plMap<const plGameObject*, plDGMLGraph::NodeId> m_VisitedObjects;
  };

  GraphVisitor visitor(ref_graph);
  pWorld->Traverse(plWorld::VisitorFunc(&GraphVisitor::Visit, &visitor), plWorld::BreadthFirst);
}


