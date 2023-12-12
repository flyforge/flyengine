#pragma once

#include <EditorTest/EditorTestPCH.h>

#include <EditorTest/TestClass/TestClass.h>

class PlasmaEditorSceneDocumentTest : public PlasmaEditorTest
{
public:
  using SUPER = PlasmaEditorTest;

  virtual const char* GetTestName() const override;

private:
  enum SubTests
  {
    ST_LayerOperations,
    ST_PrefabOperations,
    ST_ComponentOperations,
    ST_ObjectPropertyPath,
  };

  virtual void SetupSubTests() override;
  virtual plResult InitializeTest() override;
  virtual plResult DeInitializeTest() override;
  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override;

  plResult CreateSimpleScene(const char* szSceneName);
  void CloseSimpleScene();
  void LayerOperations();
  void PrefabOperations();
  void ComponentOperations();
  //void ObjectPropertyPath();

  static void CheckHierarchy(plObjectAccessorBase* pAccessor, const plDocumentObject* pRoot, plDelegate<void(const plDocumentObject* pChild)> functor);

private:
  plScene2Document* m_pDoc = nullptr;
  plLayerDocument* m_pLayer = nullptr;
  plUuid m_SceneGuid;
  plUuid m_LayerGuid;
};
