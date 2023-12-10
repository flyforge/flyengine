#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveDataDir.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(FileservePlugin, FileservePluginMain)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plFileSystem::RegisterDataDirectoryFactory(plDataDirectory::FileserveType::Factory, 100.0f);

    if (plStartup::HasApplicationTag("tool") ||
        plStartup::HasApplicationTag("testframework")) // the testframework configures a fileserve client itself
      return;

    plFileserveClient* fs = plFileserveClient::GetSingleton();

    if (fs == nullptr)
    {
      fs = PLASMA_DEFAULT_NEW(plFileserveClient);

      // on sandboxed platforms we must go through fileserve, so we enforce a fileserve connection
      // on unrestricted platforms, we use fileserve, if a connection can be established,
      // but if the connection times out, we fall back to regular file accesses
#if PLASMA_DISABLED(PLASMA_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
      if (fs->SearchForServerAddress().Failed())
      {
        fs->WaitForServerInfo().IgnoreResult();
      }
#endif
    }
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (plStartup::HasApplicationTag("tool") ||
        plStartup::HasApplicationTag("testframework"))
      return;

    if (plFileserveClient::GetSingleton() != nullptr)
    {
      plFileserveClient* pSingleton = plFileserveClient::GetSingleton();
      PLASMA_DEFAULT_DELETE(pSingleton);
    }
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on


PLASMA_STATICLINK_FILE(FileservePlugin, FileservePlugin_Main);
