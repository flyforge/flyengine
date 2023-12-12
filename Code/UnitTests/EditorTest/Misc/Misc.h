#pragma once

#include <EditorTest/EditorTestPCH.h>

#include "../TestClass/TestClass.h"

class plDocument;

class PlasmaEditorTestMisc : public PlasmaEditorTest
{
public:
  using SUPER = PlasmaEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    GameObjectReferences,
  };

  virtual void SetupSubTests() override;
  virtual plResult InitializeTest() override;
  virtual plResult DeInitializeTest() override;
  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override;

  virtual plResult InitializeSubTest(plInt32 iIdentifier) override;
  virtual plResult DeInitializeSubTest(plInt32 iIdentifier) override;

  plDocument* m_pDocument = nullptr;
};
