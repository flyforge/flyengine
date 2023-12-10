#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Utilities/Progress.h>
#include <GameEngine/GameEngineDLL.h>

using plCollectionResourceHandle = plTypedResourceHandle<class plCollectionResource>;

/// \brief This class allows to load a scene in the background and switch to it, once loading has finished.
class PLASMA_GAMEENGINE_DLL plSceneLoadUtility
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plSceneLoadUtility);

public:
  plSceneLoadUtility();
  ~plSceneLoadUtility();

  enum class LoadingState
  {
    NotStarted,
    Ongoing,
    FinishedSuccessfully,
    Failed,
  };

  /// \brief Returns whether loading is still ongoing or finished.
  LoadingState GetLoadingState() const { return m_LoadingState; }

  /// \brief Returns a loading progress value in 0 to 1 range.
  float GetLoadingProgress() const { return m_fLoadingProgress; }

  /// \brief In case loading failed, this returns what went wrong.
  plStringView GetLoadingFailureReason() const { return m_sFailureReason; }

  /// \brief Starts loading a scene. If provided, the assets in the collection are loaded first and then the scene is instantiated.
  ///
  /// Using a collection will make loading in the background much smoother. Without it, most assets will be loaded once the scene gets updated
  /// for the first time, resulting in very long delays.
  void StartSceneLoading(plStringView sSceneFile, plStringView sPreloadCollectionFile);

  /// \brief This has to be called periodically (usually once per frame) to progress the scene loading.
  ///
  /// Call GetLoadingState() afterwards to check whether loading has finished or failed.
  void TickSceneLoading();

  /// \brief Once loading is finished successfully, call this to take ownership of the loaded scene.
  ///
  /// Afterwards there is no point in keeping the plSceneLoadUtility around anymore and it should be deleted.
  plUniquePtr<plWorld> RetrieveLoadedScene();

private:
  void LoadingFailed(const plFormatString& reason);

  LoadingState m_LoadingState = LoadingState::NotStarted;
  float m_fLoadingProgress = 0.0f;
  plString m_sFailureReason;

  plString m_sFile;
  plCollectionResourceHandle m_hPreloadCollection;
  plFileReader m_FileReader;
  plWorldReader m_WorldReader;
  plUniquePtr<plWorld> m_pWorld;
  plUniquePtr<plWorldReader::InstantiationContextBase> m_pInstantiationContext;
  plProgress m_InstantiationProgress;
};
