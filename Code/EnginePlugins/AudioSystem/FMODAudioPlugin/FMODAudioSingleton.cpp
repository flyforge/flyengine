#include <FMODAudioPlugin/FMODAudioPluginPCH.h>

#include <FMODAudioPlugin/FMODAudioSingleton.h>
#include <FMODAudioPlugin/Core/FMODAudioData.h>
#include <AmplitudeAudioPlugin/Core/Common.h>

#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>

#include <Core/ResourceManager/ResourceManager.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

#include "FMODAudioSingleton.h"
#include <GameEngine/GameApplication/GameApplication.h>

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT) && PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
HANDLE g_hLiveUpdateMutex = NULL;
#endif


plResult plFMOD::Startup()
{
  if(m_bInitialized)
    return PLASMA_FAILURE;

  m_pData = PLASMA_DEFAULT_NEW(Data);

  DetectPlatform();

  if (m_pData->m_Configs.m_AssetProfiles.IsEmpty())
  {
    m_pData->m_Configs.Load(plFmodAssetProfiles::s_sConfigFile).IgnoreResult();

    if (m_pData->m_Configs.m_AssetProfiles.IsEmpty())
    {
      plLog::Warning("No valid Fmod configuration file available in '{0}'. Fmod will be deactivated.", plFmodAssetProfiles::s_sConfigFile);
      return PLASMA_FAILURE;
    }
  }

  if (!m_pData->m_Configs.m_AssetProfiles.Find(m_pData->m_sPlatform).IsValid())
  {
    plLog::Error("Fmod configuration for platform '{0}' not available. Fmod will be deactivated.", m_pData->m_sPlatform);
    return PLASMA_FAILURE;
  }

  const auto& config = m_pData->m_Configs.m_AssetProfiles[m_pData->m_sPlatform];

  FMOD_SPEAKERMODE fmodMode = FMOD_SPEAKERMODE_5POINT1;
  {
    plString sMode = "Unknown";
    switch (config.m_SpeakerMode)
    {
      case plFmodSpeakerMode::ModeStereo:
        sMode = "Stereo";
        fmodMode = FMOD_SPEAKERMODE_STEREO;
        break;
      case plFmodSpeakerMode::Mode5Point1:
        sMode = "5.1";
        fmodMode = FMOD_SPEAKERMODE_5POINT1;
        break;
      case plFmodSpeakerMode::Mode7Point1:
        sMode = "7.1";
        fmodMode = FMOD_SPEAKERMODE_7POINT1;
        break;
    }

    PLASMA_LOG_BLOCK("Fmod Configuration");
    plLog::Dev("Platform = '{0}', Mode = {1}, Channels = {2}, SamplerRate = {3}", m_pData->m_sPlatform, sMode, config.m_uiVirtualChannels, config.m_uiSamplerRate);
    plLog::Dev("Master Bank = '{0}'", config.m_sMasterSoundBank);
  }

  PLASMA_FMOD_ASSERT(FMOD::Studio::System::create(&m_pStudioSystem));

  // The example Studio project is authored for 5.1 sound, so set up the system output mode to match
  PLASMA_FMOD_ASSERT(m_pStudioSystem->getCoreSystem(&m_pLowLevelSystem));
  PLASMA_FMOD_ASSERT(m_pLowLevelSystem->setSoftwareFormat(config.m_uiSamplerRate, fmodMode, 0));

  void* extraDriverData = nullptr;
  FMOD_STUDIO_INITFLAGS studioflags = FMOD_STUDIO_INIT_NORMAL;

  // Fmod live update doesn't work with multiple instances and the same default IP
  // bank loading fails, once two processes are running that use this feature with the same IP
  // this could be reconfigured through the advanced settings, but for now we just enable live update for the first process
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  {
#  if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    // mutex handle will be closed automatically on process termination
    GetLastError(); // clear any pending error codes
    g_hLiveUpdateMutex = CreateMutexW(nullptr, TRUE, L"ezFmodLiveUpdate");

    DWORD err = GetLastError();
    if (g_hLiveUpdateMutex != NULL && err != ERROR_ALREADY_EXISTS)
    {
      studioflags |= FMOD_STUDIO_INIT_LIVEUPDATE;
    }
    else
    {
      plLog::Warning("Fmod Live-Update not available for this process, another process using Fmod is already running.");
      if (g_hLiveUpdateMutex != NULL)
      {
        CloseHandle(g_hLiveUpdateMutex); // we didn't create it, so don't keep it alive
        g_hLiveUpdateMutex = NULL;
      }
    }
#  else
    studioflags |= FMOD_STUDIO_INIT_LIVEUPDATE;
#  endif
  }
#endif

  PLASMA_FMOD_ASSERT(m_pStudioSystem->initialize(config.m_uiVirtualChannels, studioflags, FMOD_INIT_NORMAL, extraDriverData));

  if ((studioflags & FMOD_STUDIO_INIT_LIVEUPDATE) != 0)
  {
    plLog::Success("Fmod Live-Update is enabled for this process.");
  }

//   if (LoadMasterSoundBank(config.m_sMasterSoundBank).Failed())
//   {
//     plLog::Error("Failed to load Fmod master sound bank '{0}'. Sounds will not play.", config.m_sMasterSoundBank);
//     return;
//   }

  m_bInitialized = true;

  return PLASMA_SUCCESS;
}

void plFMOD::DetectPlatform()
{
  if (!m_pData->m_sPlatform.IsEmpty())
    return;

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
  m_pData->m_sPlatform = "Desktop";

#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  m_pData->m_sPlatform = "Desktop"; /// \todo Need to detect mobile device mode

#elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
  m_pData->m_sPlatform = "Desktop"; /// \todo Need to detect mobile device mode (Android)

#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX)
  m_pData->m_sPlatform = "Desktop";

#elif PLASMA_ENABLED(PLASMA_PLATFORM_IOS)
  m_pData->m_sPlatform = "Mobile";

#elif PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
  m_pData->m_sPlatform = "Mobile";
#elif
#  error "Unknown Platform"

#endif
}
