#pragma once

#include <Core/World/Declarations.h>
#include <Utilities/UtilitiesDLL.h>

class plWorld;
class plDGMLGraph;

/// \brief This class encapsulates creating graphs from various core engine structures (like the game object graph etc.)
class PLASMA_UTILITIES_DLL plDGMLGraphCreator
{
public:
  /// \brief Adds the world hierarchy (game objects and components) to the given graph object.
  static void FillGraphFromWorld(plWorld* pWorld, plDGMLGraph& Graph);
};
