
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>

class plProcessingStream;

/// \brief This element spawner initializes new elements with 0 (by writing 0 bytes into the whole element)
class PLASMA_FOUNDATION_DLL plProcessingStreamSpawnerZeroInitialized : public plProcessingStreamProcessor
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProcessingStreamSpawnerZeroInitialized, plProcessingStreamProcessor);

public:
  plProcessingStreamSpawnerZeroInitialized();

  /// \brief Which stream to zero initialize
  void SetStreamName(plStringView sStreamName);

protected:
  virtual plResult UpdateStreamBindings() override;

  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;
  virtual void Process(plUInt64 uiNumElements) override {}

  plHashedString m_sStreamName;

  plProcessingStream* m_pStream = nullptr;
};
