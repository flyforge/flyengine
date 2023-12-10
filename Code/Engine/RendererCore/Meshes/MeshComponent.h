#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

struct plMsgExtractGeometry;
using plMeshComponentManager = plComponentManager<class plMeshComponent, plBlockStorageType::Compact>;

class PLASMA_RENDERERCORE_DLL plMeshComponent : public plMeshComponentBase
{
  PLASMA_DECLARE_COMPONENT_TYPE(plMeshComponent, plMeshComponentBase, plMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plMeshComponent

public:
  plMeshComponent();
  ~plMeshComponent();

  /// \brief Extracts the render geometry for export etc.
  void OnMsgExtractGeometry(plMsgExtractGeometry& ref_msg) const; // [ msg handler ]
};
