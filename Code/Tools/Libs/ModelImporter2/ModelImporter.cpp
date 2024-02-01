#include <ModelImporter2/ModelImporterPCH.h>

#include <ModelImporter2/ImporterAssimp/ImporterAssimp.h>
#include <ModelImporter2/ImporterMagicaVoxel/ImporterMagicaVoxel.h>
#include <ModelImporter2/ImporterSourceBSP/ImporterSourceBSP.h>
#include <ModelImporter2/ModelImporter.h>

namespace plModelImporter2
{

  plUniquePtr<Importer> RequestImporterForFileType(const char* szFile)
  {
    plStringBuilder sFile = szFile;

    if (sFile.HasExtension(".fbx") || sFile.HasExtension(".obj") || sFile.HasExtension(".gltf") || sFile.HasExtension(".glb") || sFile.HasExtension(".blend"))
    {
      return PL_DEFAULT_NEW(ImporterAssimp);
    }

    if (sFile.HasExtension(".bsp"))
    {
      return PL_DEFAULT_NEW(ImporterSourceBSP);
    }

    if (sFile.HasExtension(".vox"))
    {
      return PL_DEFAULT_NEW(ImporterMagicaVoxel);
    }

    return nullptr;
  }


} // namespace plModelImporter2
