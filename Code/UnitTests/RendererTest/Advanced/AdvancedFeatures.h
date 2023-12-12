#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class plRendererTestAdvancedFeatures : public plGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "AdvancedFeatures"; }

private:
  enum SubTests
  {
    ST_ReadRenderTarget,
    ST_VertexShaderRenderTargetArrayIndex,
    //ST_BackgroundResourceLoading,
  };

  enum ImageCaptureFrames
  {
    DefaultCapture = 5,

  };

  virtual void SetupSubTests() override
  {
    AddSubTest("01 - ReadRenderTarget", SubTests::ST_ReadRenderTarget);
    AddSubTest("02 - VertexShaderRenderTargetArrayIndex", SubTests::ST_VertexShaderRenderTargetArrayIndex);
  }

  virtual plResult InitializeSubTest(plInt32 iIdentifier) override;
  virtual plResult DeInitializeSubTest(plInt32 iIdentifier) override;
  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override;

  void RenderToScreen(plUInt32 uiRenderTargetClearMask, plRectFloat viewport, plDelegate<void(plGALRenderCommandEncoder*)> func);
  void RenderCube(plRectFloat viewport, plMat4 mMVP, plUInt32 uiRenderTargetClearMask, plGALResourceViewHandle hSRV);

  void ReadRenderTarget();
  void VertexShaderRenderTargetArrayIndex();

private:
  plInt32 m_iFrame = 0;
  bool m_bCaptureImage = false;
  plHybridArray<plUInt32, 8> m_ImgCompFrames;

  plShaderResourceHandle m_hShader2;

  plGALTextureHandle m_hTexture2D;
  plGALResourceViewHandle m_hTexture2DMips[4];
  plGALTextureHandle m_hTexture2DArray;

  plMeshBufferResourceHandle m_hCubeUV;
};
