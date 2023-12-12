#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class plStringBuilder;
class plStreamWriter;
class plProgress;

//////////////////////////////////////////////////////////////////////////

/// \brief Proxy long ops represent a long operation on the editor side.
///
/// Proxy long ops have little functionality other than naming which plLongOpWorker to execute
/// in the engine process and to feed it with the necessary parameters.
/// Since the proxy long op runs in the editor process, it may access plDocumentObject's
/// and extract data from them.
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plLongOpProxy : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLongOpProxy, plReflectedClass);

public:
  /// \brief Called once by plLongOpControllerManager::RegisterLongOp() to inform the proxy
  /// to which plDocument and component (plDocumentObject) it is linked.
  virtual void InitializeRegistered(const plUuid& documentGuid, const plUuid& componentGuid) {}

  /// \brief Called by the plQtLongOpsPanel to determine the display string to be shown in the UI.
  virtual const char* GetDisplayName() const = 0;

  /// \brief Called every time the long op shall be executed
  /// \param out_sReplicationOpType must name the plLongOpWorker that shall be executed in the engine process.
  /// \param config can be optionally written to. The data is transmitted to the plLongOpWorker on the other side
  /// and fed to it in plLongOpWorker::InitializeExecution().
  virtual void GetReplicationInfo(plStringBuilder& out_sReplicationOpType, plStreamWriter& config) = 0;

  /// \brief Called once the corresponding plLongOpWorker has finished.
  /// \param result Whether the operation succeeded or failed (e.g. via user cancellation).
  /// \param resultData Optional data written by plLongOpWorker::Execute().
  virtual void Finalize(plResult result, const plDataBuffer& resultData) {}
};

//////////////////////////////////////////////////////////////////////////

/// \brief Worker long ops are executed by the editor engine process.
///
/// They typically do the actual long processing. Since they run in the engine process, they have access
/// to the runtime scene graph and resources but not the editor representation of the scene.
///
/// plLongOpWorker instances are automatically instantiated by plLongOpWorkerManager when they have
/// been named by a plLongOpProxy's GetReplicationInfo() function.
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plLongOpWorker : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLongOpWorker, plReflectedClass);

public:
  /// \brief Called within the engine processes main thread.
  /// The function may lock the plWorld from the given scene document and extract vital information.
  /// It should try to be as quick as possible and leave the heavy lifting to Execute(), which will run on a background thread.
  /// If this function return failure, the long op is canceled right away.
  virtual plResult InitializeExecution(plStreamReader& config, const plUuid& DocumentGuid) { return PLASMA_SUCCESS; }

  /// \brief Executed in a separete thread after InitializeExecution(). This should do the work that takes a while.
  ///
  /// This function may write the result data directly to disk. Everything that is written to \a proxydata
  /// will be transmitted back to the proxy long op and given to plLongOpProxy::Finalize(). Since this requires IPC bandwidth
  /// the amount of data should be kept very small (a few KB at most).
  ///
  /// All updates to \a progress will be automatically synchronized back to the editor process and become visible through
  /// the plLongOpControllerManager via the plLongOpControllerEvent.
  /// Use plProgressRange for convenient progress updates.
  virtual plResult Execute(plProgress& progress, plStreamWriter& proxydata) = 0;
};
