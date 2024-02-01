#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

struct plMsgExtractGeometry;
using plMeshComponentManager = plComponentManager<class plMeshComponent, plBlockStorageType::Compact>;

/// \brief Renders a single instance of a static mesh.
///
/// This is the main component to use for rendering regular meshes.
class PL_RENDERERCORE_DLL plMeshComponent : public plMeshComponentBase
{
  PL_DECLARE_COMPONENT_TYPE(plMeshComponent, plMeshComponentBase, plMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plMeshComponent

public:
  plMeshComponent();
  ~plMeshComponent();

  /// \brief Extracts the render geometry for export etc.
  void OnMsgExtractGeometry(plMsgExtractGeometry& ref_msg) const; // [ msg handler ]
};
