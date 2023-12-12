#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"
#include <Core/Graphics/Camera.h>

plResult plRendererTestBasics::InitializeSubTest(plInt32 iIdentifier)
{
  m_iFrame = -1;

  if (plGraphicsTest::InitializeSubTest(iIdentifier).Failed())
    return PLASMA_FAILURE;

  if (SetupRenderer().Failed())
    return PLASMA_FAILURE;

  if (iIdentifier == SubTests::ST_ClearScreen)
  {
    return CreateWindow(320, 240);
  }

  if (CreateWindow().Failed())
    return PLASMA_FAILURE;

  m_hSphere = CreateSphere(3, 1.0f);
  m_hSphere2 = CreateSphere(1, 0.75f);
  m_hTorus = CreateTorus(16, 0.5f, 0.75f);
  m_hLongBox = CreateBox(0.4f, 0.2f, 2.0f);
  m_hLineBox = CreateLineBox(0.4f, 0.2f, 2.0f);



  return PLASMA_SUCCESS;
}

plResult plRendererTestBasics::DeInitializeSubTest(plInt32 iIdentifier)
{
  m_hSphere.Invalidate();
  m_hSphere2.Invalidate();
  m_hTorus.Invalidate();
  m_hLongBox.Invalidate();
  m_hLineBox.Invalidate();
  m_hTexture2D.Invalidate();
  m_hTextureCube.Invalidate();

  DestroyWindow();
  ShutdownRenderer();

  if (plGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}


plTestAppRun plRendererTestBasics::SubtestClearScreen()
{
  BeginFrame();

  switch (m_iFrame)
  {
    case 0:
      ClearScreen(plColor(1, 0, 0));
      break;
    case 1:
      ClearScreen(plColor(0, 1, 0));
      break;
    case 2:
      ClearScreen(plColor(0, 0, 1));
      break;
    case 3:
      ClearScreen(plColor(0.5f, 0.5f, 0.5f, 0.5f));
      break;
  }

  PLASMA_TEST_IMAGE(m_iFrame, 1);

  EndFrame();

  return m_iFrame < 3 ? plTestAppRun::Continue : plTestAppRun::Quit;
}

void plRendererTestBasics::RenderObjects(plBitflags<plShaderBindFlags> ShaderBindFlags)
{
  plCamera cam;
  cam.SetCameraMode(plCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
  cam.LookAt(plVec3(0, 0, 0), plVec3(0, 0, -1), plVec3(0, 1, 0));
  plMat4 mProj;
  cam.GetProjectionMatrix((float)GetResolution().width / (float)GetResolution().height, mProj);
  plMat4 mView = cam.GetViewMatrix();

  plMat4 mTransform, mOther, mRot;

  mRot.SetRotationMatrixX(plAngle::Degree(-90));

  mOther.SetScalingMatrix(plVec3(1.0f, 1.0f, 1.0f));
  mTransform.SetTranslationMatrix(plVec3(-0.3f, -0.3f, 0.0f));
  RenderObject(m_hLongBox, mProj * mView * mTransform * mOther, plColor(1, 0, 1, 0.25f), ShaderBindFlags);

  mOther.SetRotationMatrixX(plAngle::Degree(80.0f));
  mTransform.SetTranslationMatrix(plVec3(0.75f, 0, -1.8f));
  RenderObject(m_hTorus, mProj * mView * mTransform * mOther * mRot, plColor(1, 0, 0, 0.5f), ShaderBindFlags);

  mOther.SetIdentity();
  mTransform.SetTranslationMatrix(plVec3(0, 0.1f, -2.0f));
  RenderObject(m_hSphere, mProj * mView * mTransform * mOther, plColor(0, 1, 0, 0.75f), ShaderBindFlags);

  mOther.SetScalingMatrix(plVec3(1.5f, 1.0f, 1.0f));
  mTransform.SetTranslationMatrix(plVec3(-0.6f, -0.2f, -2.2f));
  RenderObject(m_hSphere2, mProj * mView * mTransform * mOther * mRot, plColor(0, 0, 1, 1), ShaderBindFlags);
}

void plRendererTestBasics::RenderLineObjects(plBitflags<plShaderBindFlags> ShaderBindFlags)
{
  plCamera cam;
  cam.SetCameraMode(plCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
  cam.LookAt(plVec3(0, 0, 0), plVec3(0, 0, -1), plVec3(0, 1, 0));
  plMat4 mProj;
  cam.GetProjectionMatrix((float)GetResolution().width / (float)GetResolution().height, mProj);
  plMat4 mView = cam.GetViewMatrix();

  plMat4 mTransform, mOther, mRot;

  mRot.SetRotationMatrixX(plAngle::Degree(-90));

  mOther.SetScalingMatrix(plVec3(1.0f, 1.0f, 1.0f));
  mTransform.SetTranslationMatrix(plVec3(-0.3f, -0.3f, 0.0f));
  RenderObject(m_hLineBox, mProj * mView * mTransform * mOther, plColor(1, 0, 1, 0.25f), ShaderBindFlags);
}

static plRendererTestBasics g_Test;
