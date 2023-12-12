#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class plRendererTestBasics : public plGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "Basics"; }

private:
  enum SubTests
  {
    ST_ClearScreen,
    ST_RasterizerStates,
    ST_BlendStates,
    ST_Textures2D,
    ST_Textures3D,
    ST_TexturesCube,
    ST_LineRendering,
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("Clear Screen", SubTests::ST_ClearScreen);
    AddSubTest("Rasterizer States", SubTests::ST_RasterizerStates);
    AddSubTest("Blend States", SubTests::ST_BlendStates);
    AddSubTest("2D Textures", SubTests::ST_Textures2D);
    // AddSubTest("3D Textures", SubTests::ST_Textures3D); /// \todo 3D Texture support is currently not implemented
    AddSubTest("Cube Textures", SubTests::ST_TexturesCube);
    AddSubTest("Line Rendering", SubTests::ST_LineRendering);
  }


  virtual plResult InitializeSubTest(plInt32 iIdentifier) override;
  virtual plResult DeInitializeSubTest(plInt32 iIdentifier) override;

  plTestAppRun SubtestClearScreen();
  plTestAppRun SubtestRasterizerStates();
  plTestAppRun SubtestBlendStates();
  plTestAppRun SubtestTextures2D();
  plTestAppRun SubtestTextures3D();
  plTestAppRun SubtestTexturesCube();
  plTestAppRun SubtestLineRendering();

  void RenderObjects(plBitflags<plShaderBindFlags> ShaderBindFlags);
  void RenderLineObjects(plBitflags<plShaderBindFlags> ShaderBindFlags);

  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override
  {
    ++m_iFrame;

    if (iIdentifier == SubTests::ST_ClearScreen)
      return SubtestClearScreen();

    if (iIdentifier == SubTests::ST_RasterizerStates)
      return SubtestRasterizerStates();

    if (iIdentifier == SubTests::ST_BlendStates)
      return SubtestBlendStates();

    if (iIdentifier == SubTests::ST_Textures2D)
      return SubtestTextures2D();

    if (iIdentifier == SubTests::ST_Textures3D)
      return SubtestTextures3D();

    if (iIdentifier == SubTests::ST_TexturesCube)
      return SubtestTexturesCube();

    if (iIdentifier == SubTests::ST_LineRendering)
      return SubtestLineRendering();

    return plTestAppRun::Quit;
  }

  plInt32 m_iFrame;
  plMeshBufferResourceHandle m_hSphere;
  plMeshBufferResourceHandle m_hSphere2;
  plMeshBufferResourceHandle m_hTorus;
  plMeshBufferResourceHandle m_hLongBox;
  plMeshBufferResourceHandle m_hLineBox;
  plTexture2DResourceHandle m_hTexture2D;
  plTextureCubeResourceHandle m_hTextureCube;
};
