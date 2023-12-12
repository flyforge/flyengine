#pragma once

#include <Core/Graphics/Geometry.h>
#include <Core/System/Window.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererFoundation/Device/Device.h>
#include <TestFramework/Framework/TestBaseClass.h>

#undef CreateWindow

class plImage;

struct ObjectCB
{
  plMat4 m_MVP;
  plColor m_Color;
};

class plGraphicsTest : public plTestBaseClass
{
public:
  plGraphicsTest();

  virtual plResult GetImage(plImage& ref_img) override;

protected:
  virtual void SetupSubTests() override {}
  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override { return plTestAppRun::Quit; }

  virtual plResult InitializeTest() override { return PLASMA_SUCCESS; }
  virtual plResult DeInitializeTest() override { return PLASMA_SUCCESS; }
  virtual plResult InitializeSubTest(plInt32 iIdentifier) override;
  virtual plResult DeInitializeSubTest(plInt32 iIdentifier) override;

  plSizeU32 GetResolution() const;

protected:
  plResult SetupRenderer();
  plResult CreateWindow(plUInt32 uiResolutionX = 960, plUInt32 uiResolutionY = 540);

  void ShutdownRenderer();
  void DestroyWindow();
  void ClearScreen(const plColor& color = plColor::Black);
  void SetClipSpace();

  void BeginFrame();
  void EndFrame();

  plMeshBufferResourceHandle CreateMesh(const plGeometry& geom, const char* szResourceName);
  plMeshBufferResourceHandle CreateSphere(plInt32 iSubDivs, float fRadius);
  plMeshBufferResourceHandle CreateTorus(plInt32 iSubDivs, float fInnerRadius, float fOuterRadius);
  plMeshBufferResourceHandle CreateBox(float fWidth, float fHeight, float fDepth);
  plMeshBufferResourceHandle CreateLineBox(float fWidth, float fHeight, float fDepth);
  void RenderObject(plMeshBufferResourceHandle hObject, const plMat4& mTransform, const plColor& color, plBitflags<plShaderBindFlags> ShaderBindFlags = plShaderBindFlags::Default);

  plWindow* m_pWindow = nullptr;
  plGALDevice* m_pDevice = nullptr;
  plGALSwapChainHandle m_hSwapChain;
  plGALPass* m_pPass = nullptr;

  plConstantBufferStorageHandle m_hObjectTransformCB;
  plShaderResourceHandle m_hShader;
  plGALTextureHandle m_hDepthStencilTexture;
};
