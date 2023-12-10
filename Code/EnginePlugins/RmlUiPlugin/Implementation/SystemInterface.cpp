#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Time/Clock.h>
#include <RmlUiPlugin/Implementation/SystemInterface.h>

namespace plRmlUiInternal
{
  double SystemInterface::GetElapsedTime() { return plClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(); }

  void SystemInterface::JoinPath(Rml::String& translated_path, const Rml::String& document_path, const Rml::String& path)
  {
    if (plFileSystem::ExistsFile(path.c_str()))
    {
      // path is already a valid path for pl file system so don't join with document path
      translated_path = path;
      return;
    }

    Rml::SystemInterface::JoinPath(translated_path, document_path, path);
  }

  bool SystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message)
  {
    switch (type)
    {
      case Rml::Log::LT_ERROR:
        plLog::Error("{}", message.c_str());
        break;

      case Rml::Log::LT_ASSERT:
        PLASMA_REPORT_FAILURE(message.c_str());
        break;

      case Rml::Log::LT_WARNING:
        plLog::Warning("{}", message.c_str());
        break;

      case Rml::Log::LT_ALWAYS:
      case Rml::Log::LT_INFO:
        plLog::Info("{}", message.c_str());
        break;

      case Rml::Log::LT_DEBUG:
        plLog::Debug("{}", message.c_str());
        break;
      default:
        break;
    }

    return true;
  }

} // namespace plRmlUiInternal
