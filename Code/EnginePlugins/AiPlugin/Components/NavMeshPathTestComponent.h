#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

using plNavMeshPathTestComponentManager = plComponentManagerSimple<class plAiNavMeshPathTestComponent, plComponentUpdateType::WhenSimulating>;

/// \brief Used to test path-finding through a navmesh.
///
/// The component takes a reference to another game object as the destination
/// and then requests a path from the navmesh.
/// Various aspects of the path can be visualized for inspection.
///
/// This component should be used in the editor, to test whether the scene navmesh behaves as desired.
class PL_AIPLUGIN_DLL plAiNavMeshPathTestComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plAiNavMeshPathTestComponent, plComponent, plNavMeshPathTestComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  //  plAiNavMeshPathTestComponent

public:
  plAiNavMeshPathTestComponent();
  ~plAiNavMeshPathTestComponent();

  void SetPathEndReference(const char* szReference); // [ property ]
  void SetPathEnd(plGameObjectHandle hObject);

  /// \brief Render the navmesh polygons, through which the path goes.
  bool m_bVisualizePathCorridor = true; // [ property ]

  /// \brief Render a line for the shortest path through the corridor.
  bool m_bVisualizePathLine = true;   // [ property ]

  /// \brief Render text describing what went wrong during path search.
  bool m_bVisualizePathState = true;  // [ property ]

  /// \brief Name of the plAiNavmeshConfig to use. See plAiNavigationConfig.
  plHashedString m_sNavmeshConfig;    // [ property ]

  /// \brief Name of the plAiPathSearchConfig to use. See plAiNavigationConfig.
  plHashedString m_sPathSearchConfig; // [ property ]

protected:
  void Update();

  plGameObjectHandle m_hPathEnd;
  plAiNavigation m_Navigation;

private:
  const char* DummyGetter() const { return nullptr; }
};
