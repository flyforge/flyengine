#include <ModelImporter2/ModelImporterPCH.h>

#include <Foundation/Logging/Log.h>
#include <ModelImporter2/Importer/Importer.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

namespace plModelImporter2
{
  Importer::Importer() = default;
  Importer::~Importer() = default;

  plResult Importer::Import(const ImportOptions& options, plLogInterface* pLogInterface /*= nullptr*/, plProgress* pProgress /*= nullptr*/)
  {
    plResult res = PLASMA_FAILURE;

    plLogInterface* pPrevLogSystem = plLog::GetThreadLocalLogSystem();

    if (pLogInterface)
    {
      plLog::SetThreadLocalLogSystem(pLogInterface);
    }

    {
      m_pProgress = pProgress;
      m_Options = options;

      PLASMA_LOG_BLOCK("ModelImport", m_Options.m_sSourceFile);

      res = DoImport();
    }


    plLog::SetThreadLocalLogSystem(pPrevLogSystem);

    return res;
  }

} // namespace plModelImporter2
