#pragma once

#include <ModelImporter2/Importer/Importer.h>

namespace plModelImporter2
{
  /// Importer implementation to import Source engine BSP files.
  class ImporterMagicaVoxel : public Importer
  {
  public:
    ImporterMagicaVoxel();
    ~ImporterMagicaVoxel();

  protected:
    virtual plResult DoImport() override;
  };
} // namespace plModelImporter2
