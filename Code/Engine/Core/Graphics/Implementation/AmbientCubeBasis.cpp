#include <Core/CorePCH.h>

#include <Core/Graphics/AmbientCubeBasis.h>

plVec3 plAmbientCubeBasis::s_Dirs[NumDirs] = {plVec3(1.0f, 0.0f, 0.0f), plVec3(-1.0f, 0.0f, 0.0f), plVec3(0.0f, 1.0f, 0.0f),
  plVec3(0.0f, -1.0f, 0.0f), plVec3(0.0f, 0.0f, 1.0f), plVec3(0.0f, 0.0f, -1.0f)};


PLASMA_STATICLINK_FILE(Core, Core_Graphics_Implementation_AmbientCubeBasis);
