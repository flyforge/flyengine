#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/Declarations.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/LogEntry.h>

class PL_EDITORFRAMEWORK_DLL plProcessAssetMsg : public plProcessMessage
{
  PL_ADD_DYNAMIC_REFLECTION(plProcessAssetMsg, plProcessMessage);

public:
  plUuid m_AssetGuid;
  plUInt64 m_AssetHash = 0;
  plUInt64 m_ThumbHash = 0;
  plString m_sAssetPath;
  plString m_sPlatform;
  plDynamicArray<plString> m_DepRefHull;
};

class PL_EDITORFRAMEWORK_DLL plProcessAssetResponseMsg : public plProcessMessage
{
  PL_ADD_DYNAMIC_REFLECTION(plProcessAssetResponseMsg, plProcessMessage);

public:
  plTransformStatus m_Status;
  mutable plDynamicArray<plLogEntry> m_LogEntries;
};
