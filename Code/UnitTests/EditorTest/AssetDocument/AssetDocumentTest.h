#pragma once

#include <EditorTest/EditorTestPCH.h>

#include <EditorTest/TestClass/TestClass.h>

class PlasmaEditorAssetDocumentTest : public PlasmaEditorTest
{
public:
  using SUPER = PlasmaEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_AsyncSave,
    ST_SaveOnTransform,
  };

  virtual void SetupSubTests() override;
  virtual plResult InitializeTest() override;
  virtual plResult DeInitializeTest() override;
  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override;

  void AsyncSave();
  void SaveOnTransform();
};
