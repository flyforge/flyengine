#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Time/Clock.h>
#include <RmlUiPlugin/Implementation/SystemInterface.h>

namespace plRmlUiInternal
{
  double SystemInterface::GetElapsedTime() { return plClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(); }

  void SystemInterface::JoinPath(Rml::String& ref_sTranslated_path, const Rml::String& sDocument_path, const Rml::String& sPath)
  {
    if (plFileSystem::ExistsFile(sPath.c_str()))
    {
      // path is already a valid path for pl file system so don't join with document path
      ref_sTranslated_path = sPath;
      return;
    }

    Rml::SystemInterface::JoinPath(ref_sTranslated_path, sDocument_path, sPath);
  }

  bool SystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& sMessage)
  {
    switch (type)
    {
      case Rml::Log::LT_ERROR:
        plLog::Error("{}", sMessage.c_str());
        break;

      case Rml::Log::LT_ASSERT:
        PL_REPORT_FAILURE(sMessage.c_str());
        break;

      case Rml::Log::LT_WARNING:
        plLog::Warning("{}", sMessage.c_str());
        break;

      case Rml::Log::LT_ALWAYS:
      case Rml::Log::LT_INFO:
        plLog::Info("{}", sMessage.c_str());
        break;

      case Rml::Log::LT_DEBUG:
        plLog::Debug("{}", sMessage.c_str());
        break;
      default:
        break;
    }

    return true;
  }

} // namespace plRmlUiInternal
