#include <Utilities/UtilitiesPCH.h>

PLASMA_STATICLINK_LIBRARY(Utilities)
{
  if (bReturn)
    return;

  PLASMA_STATICLINK_REFERENCE(Utilities_DGML_Implementation_DGMLCreator);
  PLASMA_STATICLINK_REFERENCE(Utilities_DataStructures_Implementation_DynamicOctree);
  PLASMA_STATICLINK_REFERENCE(Utilities_DataStructures_Implementation_DynamicQuadtree);
  PLASMA_STATICLINK_REFERENCE(Utilities_DataStructures_Implementation_ObjectSelection);
  PLASMA_STATICLINK_REFERENCE(Utilities_FileFormats_Implementation_OBJLoader);
  PLASMA_STATICLINK_REFERENCE(Utilities_GridAlgorithms_Implementation_Rasterization);
  PLASMA_STATICLINK_REFERENCE(Utilities_PathFinding_Implementation_GridNavmesh);
}
