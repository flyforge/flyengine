
#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/DataProcessing/Stream/DefaultImplementations/ZeroInitializer.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(DataProcessing);

// Add processor

class AddOneStreamProcessor : public plProcessingStreamProcessor
{
  PLASMA_ADD_DYNAMIC_REFLECTION(AddOneStreamProcessor, plProcessingStreamProcessor);

public:
  AddOneStreamProcessor()

    = default;

  void SetStreamName(plHashedString sStreamName) { m_sStreamName = sStreamName; }

protected:
  virtual plResult UpdateStreamBindings() override
  {
    m_pStream = m_pStreamGroup->GetStreamByName(m_sStreamName);

    return m_pStream ? PLASMA_SUCCESS : PLASMA_FAILURE;
  }

  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override {}

  virtual void Process(plUInt64 uiNumElements) override
  {
    plProcessingStreamIterator<float> streamIterator(m_pStream, uiNumElements, 0);

    while (!streamIterator.HasReachedEnd())
    {
      streamIterator.Current() += 1.0f;

      streamIterator.Advance();
    }
  }

  plHashedString m_sStreamName;
  plProcessingStream* m_pStream = nullptr;
};

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(AddOneStreamProcessor, 1, plRTTIDefaultAllocator<AddOneStreamProcessor>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_CREATE_SIMPLE_TEST(DataProcessing, ProcessingStream)
{
  plProcessingStreamGroup Group;
  plProcessingStream* pStream1 = Group.AddStream("Stream1", plProcessingStream::DataType::Float);
  plProcessingStream* pStream2 = Group.AddStream("Stream2", plProcessingStream::DataType::Float3);

  PLASMA_TEST_BOOL(pStream1 != nullptr);
  PLASMA_TEST_BOOL(pStream2 != nullptr);

  plProcessingStreamSpawnerZeroInitialized* pSpawner1 = PLASMA_DEFAULT_NEW(plProcessingStreamSpawnerZeroInitialized);
  plProcessingStreamSpawnerZeroInitialized* pSpawner2 = PLASMA_DEFAULT_NEW(plProcessingStreamSpawnerZeroInitialized);

  pSpawner1->SetStreamName(pStream1->GetName());
  pSpawner2->SetStreamName(pStream2->GetName());

  Group.AddProcessor(pSpawner1);
  Group.AddProcessor(pSpawner2);

  Group.SetSize(128);

  PLASMA_TEST_INT(Group.GetNumElements(), 128);
  PLASMA_TEST_INT(Group.GetNumActiveElements(), 0);

  Group.InitializeElements(3);

  Group.Process();

  PLASMA_TEST_INT(Group.GetNumActiveElements(), 3);


  {
    plProcessingStreamIterator<float> stream1Iterator(pStream1, 3, 0);

    int iElementsVisited = 0;
    while (!stream1Iterator.HasReachedEnd())
    {
      PLASMA_TEST_FLOAT(stream1Iterator.Current(), 0.0f, 0.0f);

      stream1Iterator.Advance();
      iElementsVisited++;
    }

    PLASMA_TEST_INT(iElementsVisited, 3);
  }

  Group.InitializeElements(7);

  Group.Process();

  {
    plProcessingStreamIterator<plVec3> stream2Iterator(pStream2, Group.GetNumActiveElements(), 0);

    int iElementsVisited = 0;
    while (!stream2Iterator.HasReachedEnd())
    {
      PLASMA_TEST_FLOAT(stream2Iterator.Current().x, 0.0f, 0.0f);
      PLASMA_TEST_FLOAT(stream2Iterator.Current().y, 0.0f, 0.0f);
      PLASMA_TEST_FLOAT(stream2Iterator.Current().z, 0.0f, 0.0f);

      stream2Iterator.Advance();
      iElementsVisited++;
    }

    PLASMA_TEST_INT(iElementsVisited, 10);
  }

  PLASMA_TEST_INT(Group.GetHighestNumActiveElements(), 10);

  Group.RemoveElement(5);
  Group.RemoveElement(7);

  Group.Process();

  PLASMA_TEST_INT(Group.GetHighestNumActiveElements(), 10);
  PLASMA_TEST_INT(Group.GetNumActiveElements(), 8);

  AddOneStreamProcessor* pProcessor1 = PLASMA_DEFAULT_NEW(AddOneStreamProcessor);
  pProcessor1->SetStreamName(pStream1->GetName());

  Group.AddProcessor(pProcessor1);

  Group.Process();

  {
    plProcessingStreamIterator<float> stream1Iterator(pStream1, Group.GetNumActiveElements(), 0);
    while (!stream1Iterator.HasReachedEnd())
    {
      PLASMA_TEST_FLOAT(stream1Iterator.Current(), 1.0f, 0.001f);

      stream1Iterator.Advance();
    }
  }

  Group.Process();

  {
    plProcessingStreamIterator<float> stream1Iterator(pStream1, Group.GetNumActiveElements(), 0);
    while (!stream1Iterator.HasReachedEnd())
    {
      PLASMA_TEST_FLOAT(stream1Iterator.Current(), 2.0f, 0.001f);

      stream1Iterator.Advance();
    }
  }
}
