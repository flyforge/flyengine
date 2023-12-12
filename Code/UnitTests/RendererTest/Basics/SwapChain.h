#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class plRendererTestSwapChain : public plGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "SwapChain"; }

private:
  enum SubTests
  {
    ST_ColorOnly,
    ST_D16,
    ST_D24S8,
    ST_D32,
    ST_NoVSync,
    ST_ResizeWindow,
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("Color Only", SubTests::ST_ColorOnly);
    AddSubTest("Depth D16", SubTests::ST_D16);
    AddSubTest("Depth D24S8", SubTests::ST_D24S8);
    AddSubTest("Depth D32", SubTests::ST_D32);
    AddSubTest("No VSync", SubTests::ST_NoVSync);
    AddSubTest("Resize Window", SubTests::ST_ResizeWindow);
  }

  virtual plResult InitializeSubTest(plInt32 iIdentifier) override;
  virtual plResult DeInitializeSubTest(plInt32 iIdentifier) override;

  void ResizeTest(plUInt32 uiInvocationCount);
  plTestAppRun BasicRenderLoop(plInt32 iIdentifier, plUInt32 uiInvocationCount);

  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override
  {
    ++m_iFrame;

    switch (iIdentifier)
    {
      case SubTests::ST_ResizeWindow:
        ResizeTest(uiInvocationCount);
        [[fallthrough]];
      case SubTests::ST_ColorOnly:
      case SubTests::ST_D16:
      case SubTests::ST_D24S8:
      case SubTests::ST_D32:
      case SubTests::ST_NoVSync:
        return BasicRenderLoop(iIdentifier, uiInvocationCount);
      default:
        PLASMA_ASSERT_NOT_IMPLEMENTED;
        break;
    }
    return plTestAppRun::Quit;
  }

  plSizeU32 m_CurrentWindowSize = plSizeU32(320, 240);
  plInt32 m_iFrame;
};
