#pragma once

#include <ModelImporter2/Importer/Importer.h>

namespace plModelImporter2
{
  /// Importer implementation to import Source engine BSP files.
  class ImporterSourceBSP : public Importer
  {
  public:
    ImporterSourceBSP();
    ~ImporterSourceBSP();

  protected:
    virtual plResult DoImport() override;
  };
} // namespace plModelImporter2
