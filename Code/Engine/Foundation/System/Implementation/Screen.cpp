#include <Foundation/FoundationPCH.h>

#include <Foundation/System/PlatformFeatures.h>
#include <Foundation/System/Screen.h>

void plScreen::PrintScreenInfo(const plHybridArray<plScreenInfo, 2>& screens, plLogInterface* pLog /*= plLog::GetThreadLocalLogSystem()*/)
{
  PL_LOG_BLOCK(pLog, "Screens");

  plLog::Info(pLog, "Found {0} screens", screens.GetCount());

  for (const auto& screen : screens)
  {
    plLog::Dev(pLog, "'{0}': Offset = ({1}, {2}), Resolution = ({3}, {4}){5}", screen.m_sDisplayName, screen.m_iOffsetX, screen.m_iOffsetY, screen.m_iResolutionX, screen.m_iResolutionY, screen.m_bIsPrimary ? " (primary)" : "");
  }
}


