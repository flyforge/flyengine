#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"
#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>

plTestAppRun plRendererTestBasics::SubtestTextures2D()
{
  BeginFrame();

  plRenderContext::GetDefaultInstance()->SetDefaultTextureFilter(plTextureFilterSetting::FixedTrilinear);

  const plInt32 iNumFrames = 14;

  m_hShader = plResourceManager::LoadResource<plShaderResource>("RendererTest/Shaders/Textured.plShader");

  if (m_iFrame == 0)
  {
    m_hTexture2D = plResourceManager::LoadResource<plTexture2DResource>("SharedData/Textures/plLogo_ABGR_Mips_D.dds");
  }

  if (m_iFrame == 1)
  {
    m_hTexture2D = plResourceManager::LoadResource<plTexture2DResource>("SharedData/Textures/plLogo_ABGR_NoMips_D.dds");
  }

  if (m_iFrame == 2)
  {
    m_hTexture2D = plResourceManager::LoadResource<plTexture2DResource>("SharedData/Textures/plLogo_ARGB_Mips_D.dds");
  }

  if (m_iFrame == 3)
  {
    m_hTexture2D = plResourceManager::LoadResource<plTexture2DResource>("SharedData/Textures/plLogo_ARGB_NoMips_D.dds");
  }

  if (m_iFrame == 4)
  {
    m_hTexture2D = plResourceManager::LoadResource<plTexture2DResource>("SharedData/Textures/plLogo_DXT1_Mips_D.dds");
  }

  if (m_iFrame == 5)
  {
    m_hTexture2D = plResourceManager::LoadResource<plTexture2DResource>("SharedData/Textures/plLogo_DXT1_NoMips_D.dds");
  }

  if (m_iFrame == 6)
  {
    m_hTexture2D = plResourceManager::LoadResource<plTexture2DResource>("SharedData/Textures/plLogo_DXT3_Mips_D.dds");
  }

  if (m_iFrame == 7)
  {
    m_hTexture2D = plResourceManager::LoadResource<plTexture2DResource>("SharedData/Textures/plLogo_DXT3_NoMips_D.dds");
  }

  if (m_iFrame == 8)
  {
    m_hTexture2D = plResourceManager::LoadResource<plTexture2DResource>("SharedData/Textures/plLogo_DXT5_Mips_D.dds");
  }

  if (m_iFrame == 9)
  {
    m_hTexture2D = plResourceManager::LoadResource<plTexture2DResource>("SharedData/Textures/plLogo_DXT5_NoMips_D.dds");
  }

  if (m_iFrame == 10)
  {
    m_hTexture2D = plResourceManager::LoadResource<plTexture2DResource>("SharedData/Textures/plLogo_RGB_Mips_D.dds");
  }

  if (m_iFrame == 11)
  {
    m_hTexture2D = plResourceManager::LoadResource<plTexture2DResource>("SharedData/Textures/plLogo_RGB_NoMips_D.dds");
  }

  if (m_iFrame == 12)
  {
    m_hTexture2D = plResourceManager::LoadResource<plTexture2DResource>("SharedData/Textures/plLogo_R5G6B5_NoMips_D.dds");
  }

  if (m_iFrame == 13)
  {
    m_hTexture2D = plResourceManager::LoadResource<plTexture2DResource>("SharedData/Textures/plLogo_R5G6B5_MipsD.dds");
  }

  plRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", m_hTexture2D);

  ClearScreen(plColor::Black);

  RenderObjects(plShaderBindFlags::Default);

  PLASMA_TEST_IMAGE(m_iFrame, 100);

  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? plTestAppRun::Continue : plTestAppRun::Quit;
}


plTestAppRun plRendererTestBasics::SubtestTextures3D()
{
  BeginFrame();

  plRenderContext::GetDefaultInstance()->SetDefaultTextureFilter(plTextureFilterSetting::FixedTrilinear);

  const plInt32 iNumFrames = 1;

  m_hShader = plResourceManager::LoadResource<plShaderResource>("RendererTest/Shaders/TexturedVolume.plShader");

  if (m_iFrame == 0)
  {
    m_hTexture2D = plResourceManager::LoadResource<plTexture2DResource>("SharedData/Textures/Volume/plLogo_Volume_A8_NoMips_D.dds");
  }

  plRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", m_hTexture2D);

  ClearScreen(plColor::Black);

  RenderObjects(plShaderBindFlags::Default);

  PLASMA_TEST_IMAGE(m_iFrame, 100);

  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? plTestAppRun::Continue : plTestAppRun::Quit;
}


plTestAppRun plRendererTestBasics::SubtestTexturesCube()
{
  BeginFrame();

  plRenderContext::GetDefaultInstance()->SetDefaultTextureFilter(plTextureFilterSetting::FixedTrilinear);

  const plInt32 iNumFrames = 12;

  m_hShader = plResourceManager::LoadResource<plShaderResource>("RendererTest/Shaders/TexturedCube.plShader");

  if (m_iFrame == 0)
  {
    m_hTextureCube = plResourceManager::LoadResource<plTextureCubeResource>("SharedData/Textures/Cubemap/plLogo_Cube_XRGB_NoMips_D.dds");
  }

  if (m_iFrame == 1)
  {
    m_hTextureCube = plResourceManager::LoadResource<plTextureCubeResource>("SharedData/Textures/Cubemap/plLogo_Cube_XRGB_Mips_D.dds");
  }

  if (m_iFrame == 2)
  {
    m_hTextureCube = plResourceManager::LoadResource<plTextureCubeResource>("SharedData/Textures/Cubemap/plLogo_Cube_RGBA_NoMips_D.dds");
  }

  if (m_iFrame == 3)
  {
    m_hTextureCube = plResourceManager::LoadResource<plTextureCubeResource>("SharedData/Textures/Cubemap/plLogo_Cube_RGBA_Mips_D.dds");
  }

  if (m_iFrame == 4)
  {
    m_hTextureCube = plResourceManager::LoadResource<plTextureCubeResource>("SharedData/Textures/Cubemap/plLogo_Cube_DXT1_NoMips_D.dds");
  }

  if (m_iFrame == 5)
  {
    m_hTextureCube = plResourceManager::LoadResource<plTextureCubeResource>("SharedData/Textures/Cubemap/plLogo_Cube_DXT1_Mips_D.dds");
  }

  if (m_iFrame == 6)
  {
    m_hTextureCube = plResourceManager::LoadResource<plTextureCubeResource>("SharedData/Textures/Cubemap/plLogo_Cube_DXT3_NoMips_D.dds");
  }

  if (m_iFrame == 7)
  {
    m_hTextureCube = plResourceManager::LoadResource<plTextureCubeResource>("SharedData/Textures/Cubemap/plLogo_Cube_DXT3_Mips_D.dds");
  }

  if (m_iFrame == 8)
  {
    m_hTextureCube = plResourceManager::LoadResource<plTextureCubeResource>("SharedData/Textures/Cubemap/plLogo_Cube_DXT5_NoMips_D.dds");
  }

  if (m_iFrame == 9)
  {
    m_hTextureCube = plResourceManager::LoadResource<plTextureCubeResource>("SharedData/Textures/Cubemap/plLogo_Cube_DXT5_Mips_D.dds");
  }

  if (m_iFrame == 10)
  {
    m_hTextureCube = plResourceManager::LoadResource<plTextureCubeResource>("SharedData/Textures/Cubemap/plLogo_Cube_RGB_NoMips_D.dds");
  }

  if (m_iFrame == 11)
  {
    m_hTextureCube = plResourceManager::LoadResource<plTextureCubeResource>("SharedData/Textures/Cubemap/plLogo_Cube_RGB_Mips_D.dds");
  }

  plRenderContext::GetDefaultInstance()->BindTextureCube("DiffuseTexture", m_hTextureCube);

  ClearScreen(plColor::Black);

  RenderObjects(plShaderBindFlags::Default);

  PLASMA_TEST_IMAGE(m_iFrame, 100);

  EndFrame();

  return m_iFrame < (iNumFrames - 1) ? plTestAppRun::Continue : plTestAppRun::Quit;
}
