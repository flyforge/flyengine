#include <GameEngine/GameEnginePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Collection/CollectionResource.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Utils/SceneLoadUtil.h>

// preloading assets is considered to be the vast majority of scene loading
constexpr float fCollectionPreloadPiece = 0.9f;

plSceneLoadUtility::plSceneLoadUtility() = default;
plSceneLoadUtility::~plSceneLoadUtility() = default;

void plSceneLoadUtility::StartSceneLoading(plStringView sSceneFile, plStringView sPreloadCollectionFile)
{
  PLASMA_ASSERT_DEV(m_LoadingState == LoadingState::NotStarted, "Can't reuse an plSceneLoadUtility.");

  PLASMA_LOG_BLOCK("StartSceneLoading");

  m_LoadingState = LoadingState::Ongoing;

  plStringBuilder sFinalSceneFile = sSceneFile;

  if (sFinalSceneFile.IsEmpty())
  {
    LoadingFailed("No scene file specified.");
    return;
  }

  plLog::Info("Loading scene '{}'.", sSceneFile);

  if (sFinalSceneFile.IsAbsolutePath())
  {
    // this can fail if the scene is in a different data directory than the project directory
    // shouldn't stop us from loading it anyway
    sFinalSceneFile.MakeRelativeTo(plGameApplication::GetGameApplicationInstance()->GetAppProjectPath()).IgnoreResult();
  }

  if (sFinalSceneFile.HasExtension("plScene") || sFinalSceneFile.HasExtension("plPrefab"))
  {
    if (sFinalSceneFile.IsAbsolutePath())
    {
      if (plFileSystem::ResolvePath(sFinalSceneFile, nullptr, &sFinalSceneFile).Failed())
      {
        LoadingFailed(plFmt("Scene path is not located in any data directory: '{}'", sFinalSceneFile));
        return;
      }
    }

    // if this is a path to the non-transformed source file, redirect it to the transformed file in the asset cache
    sFinalSceneFile.Prepend("AssetCache/Common/");
    sFinalSceneFile.ChangeFileExtension("plObjectGraph");
  }

  if (sFinalSceneFile != sSceneFile)
  {
    plLog::Dev("Redirecting scene file from '{}' to '{}'", sSceneFile, sFinalSceneFile);
  }

  m_sFile = sFinalSceneFile;

  if (!sPreloadCollectionFile.IsEmpty())
  {
    m_hPreloadCollection = plResourceManager::LoadResource<plCollectionResource>(plString(sPreloadCollectionFile));
  }
}

plUniquePtr<plWorld> plSceneLoadUtility::RetrieveLoadedScene()
{
  PLASMA_ASSERT_DEV(m_LoadingState == LoadingState::FinishedSuccessfully, "Can't retrieve a scene when loading hasn't finished successfully.");

  return std::move(m_pWorld);
}

void plSceneLoadUtility::LoadingFailed(const plFormatString& reason)
{
  PLASMA_ASSERT_DEV(m_LoadingState == LoadingState::Ongoing, "Invalid loading state");
  m_LoadingState = LoadingState::Failed;

  plStringBuilder tmp;
  m_sFailureReason = reason.GetText(tmp);
}

void plSceneLoadUtility::TickSceneLoading()
{
  switch (m_LoadingState)
  {
    case LoadingState::FinishedSuccessfully:
    case LoadingState::Failed:
      return;

    default:
      break;
  }

  // update our current loading progress
  {
    m_fLoadingProgress = fCollectionPreloadPiece;

    // if we have a collection, preload that first
    if (m_hPreloadCollection.IsValid())
    {
      m_fLoadingProgress = 0.0f;

      plResourceLock<plCollectionResource> pCollection(m_hPreloadCollection, plResourceAcquireMode::AllowLoadingFallback_NeverFail);

      if (pCollection.GetAcquireResult() == plResourceAcquireResult::Final)
      {
        pCollection->PreloadResources();

        float progress = 0.0f;
        if (pCollection->IsLoadingFinished(&progress))
        {
          m_fLoadingProgress = fCollectionPreloadPiece;
        }
        else
        {
          m_fLoadingProgress = progress * fCollectionPreloadPiece;
        }
      }
    }

    // if preloading the collection is finished (or we just don't have one) add the world instantiation progress
    if (m_fLoadingProgress == fCollectionPreloadPiece)
    {
      m_fLoadingProgress += m_InstantiationProgress.GetCompletion() * (1.0f - fCollectionPreloadPiece);
    }
  }

  // as long as we are still pre-loading assets from the collection, don't do anything else
  if (m_fLoadingProgress < fCollectionPreloadPiece)
    return;

  // if we haven't created a world yet, do so now, and set up an instantiation context
  if (m_pWorld == nullptr)
  {
    PLASMA_LOG_BLOCK("LoadObjectGraph", m_sFile);

    plWorldDesc desc(m_sFile);
    m_pWorld = PLASMA_DEFAULT_NEW(plWorld, desc);

    PLASMA_LOCK(m_pWorld->GetWriteMarker());

    if (m_FileReader.Open(m_sFile).Failed())
    {
      LoadingFailed("Failed to open the file.");
      return;
    }
    else
    {
      // Read and skip the asset file header
      plAssetFileHeader header;
      header.Read(m_FileReader).AssertSuccess();

      char szSceneTag[16];
      m_FileReader.ReadBytes(szSceneTag, sizeof(char) * 16);

      if (!plStringUtils::IsEqualN(szSceneTag, "[plBinaryScene]", 16))
      {
        LoadingFailed("The given file isn't an object-graph file.");
        return;
      }

      if (m_WorldReader.ReadWorldDescription(m_FileReader).Failed())
      {
        LoadingFailed("Error reading world description.");
        return;
      }

      m_pInstantiationContext = m_WorldReader.InstantiateWorld(*m_pWorld, nullptr, plTime::Milliseconds(1), &m_InstantiationProgress);
    }
  }
  else if (m_pInstantiationContext)
  {
    plWorldReader::InstantiationContextBase::StepResult res = m_pInstantiationContext->Step();

    if (res == plWorldReader::InstantiationContextBase::StepResult::ContinueNextFrame)
    {
      // TODO: can we finish the world instantiation without updating the entire world?
      // E.g. only finish component instantiation?
      // also we may want to step the world only with a very small (and fixed!) time-step

      PLASMA_LOCK(m_pWorld->GetWriteMarker());
      m_pWorld->Update();
    }
    else if (res == plWorldReader::InstantiationContextBase::StepResult::Finished)
    {
      // TODO: ticking twice seems to fix some Jolt physics issues
      PLASMA_LOCK(m_pWorld->GetWriteMarker());
      m_pWorld->Update();

      m_pInstantiationContext = nullptr;
      m_LoadingState = LoadingState::FinishedSuccessfully;
    }

    m_fLoadingProgress = fCollectionPreloadPiece + m_InstantiationProgress.GetCompletion() * (1.0f - fCollectionPreloadPiece);
  }
  else
  {
    PLASMA_REPORT_FAILURE("Invalid code path.");
  }
}
