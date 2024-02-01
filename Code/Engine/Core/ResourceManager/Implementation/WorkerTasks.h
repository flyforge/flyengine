#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/UniquePtr.h>

/// \brief [internal] Worker task for loading resources (typically from disk).
class PL_CORE_DLL plResourceManagerWorkerDataLoad final : public plTask
{
public:
  ~plResourceManagerWorkerDataLoad();

private:
  friend class plResourceManager;
  friend class plResourceManagerState;

  plResourceManagerWorkerDataLoad();

  virtual void Execute() override;
};

/// \brief [internal] Worker task for uploading resource data.
/// Depending on the resource type, this may get scheduled to run on the main thread or on any thread.
class PL_CORE_DLL plResourceManagerWorkerUpdateContent final : public plTask
{
public:
  ~plResourceManagerWorkerUpdateContent();

  plResourceLoadData m_LoaderData;
  plResource* m_pResourceToLoad = nullptr;
  plResourceTypeLoader* m_pLoader = nullptr;
  // this is only used to clean up a custom loader at the right time, if one is used
  // m_pLoader is always set, no need to go through m_pCustomLoader
  plUniquePtr<plResourceTypeLoader> m_pCustomLoader;

private:
  friend class plResourceManager;
  friend class plResourceManagerState;
  friend class plResourceManagerWorkerDataLoad;
  plResourceManagerWorkerUpdateContent();

  virtual void Execute() override;
};
