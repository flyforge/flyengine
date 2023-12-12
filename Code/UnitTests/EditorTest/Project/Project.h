#pragma once

#include <EditorTest/EditorTestPCH.h>

#include "../TestClass/TestClass.h"

class PlasmaEditorTestProject : public PlasmaEditorTest
{
public:
  using SUPER = PlasmaEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_CreateDocuments,
  };

  virtual void SetupSubTests() override;
  virtual plResult InitializeTest() override;
  virtual plResult DeInitializeTest() override;
  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override;
};
