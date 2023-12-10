#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plDocument;

class PLASMA_TOOLSFOUNDATION_DLL plApplicationServices
{
  PLASMA_DECLARE_SINGLETON(plApplicationServices);

public:
  plApplicationServices();

  /// \brief A writable folder in which application specific user data may be stored
  plString GetApplicationUserDataFolder() const;

  /// \brief A read-only folder in which application specific data may be located
  plString GetApplicationDataFolder() const;

  /// \brief The writable location where the application should store preferences (user specific settings)
  plString GetApplicationPreferencesFolder() const;

  /// \brief The writable location where preferences for the current plToolsProject should be stored (user specific settings)
  plString GetProjectPreferencesFolder() const;

  plString GetProjectPreferencesFolder(plStringView sProjectFilePath) const;

  /// \brief The writable location where preferences for the given plDocument should be stored (user specific settings)
  plString GetDocumentPreferencesFolder(const plDocument* pDocument) const;

  /// \brief The read-only folder where pre-compiled binaries for external tools can be found
  plString GetPrecompiledToolsFolder(bool bUsePrecompiledTools) const;

  /// \brief The folder where under which the sample projects are stored
  plString GetSampleProjectsFolder() const;
};
