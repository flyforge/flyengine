#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <ModelImporter2/Importer/Importer.h>

namespace plModelImporter2
{
  PL_MODELIMPORTER2_DLL plUniquePtr<Importer> RequestImporterForFileType(const char* szFile);

} // namespace plModelImporter2
