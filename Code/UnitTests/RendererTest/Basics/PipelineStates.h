#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class plRendererTestPipelineStates : public plGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "PipelineStates"; }

private:
  enum SubTests
  {
    ST_MostBasicShader,
    ST_ViewportScissor,
    ST_VertexBuffer,
    ST_IndexBuffer,
    ST_ConstantBuffer,
    ST_StructuredBuffer,
    ST_Texture2D,
    ST_Texture2DArray,
    ST_GenerateMipMaps,
    ST_Timestamps,
  };

  enum ImageCaptureFrames
  {
    DefaultCapture = 5,
    StructuredBuffer_InitialData = 5,
    StructuredBuffer_Discard = 6,
    StructuredBuffer_NoOverwrite = 8,
    StructuredBuffer_CopyToTempStorage = 9,
    Timestamps_MaxWaitTime = plMath::MaxValue<plUInt32>(),
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("01 - MostBasicShader", SubTests::ST_MostBasicShader);
    AddSubTest("02 - ViewportScissor", SubTests::ST_ViewportScissor);
    AddSubTest("03 - VertexBuffer", SubTests::ST_VertexBuffer);
    AddSubTest("04 - IndexBuffer", SubTests::ST_IndexBuffer);
    AddSubTest("05 - ConstantBuffer", SubTests::ST_ConstantBuffer);
    AddSubTest("06 - StructuredBuffer", SubTests::ST_StructuredBuffer);
    AddSubTest("07 - Texture2D", SubTests::ST_Texture2D);
    AddSubTest("08 - Texture2DArray", SubTests::ST_Texture2DArray);
    AddSubTest("09 - GenerateMipMaps", SubTests::ST_GenerateMipMaps);
    //AddSubTest("10 - Timestamps", SubTests::ST_Timestamps);
  }

  virtual plResult InitializeSubTest(plInt32 iIdentifier) override;
  virtual plResult DeInitializeSubTest(plInt32 iIdentifier) override;
  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) override;

  void RenderBlock(plMeshBufferResourceHandle mesh, plColor clearColor = plColor::CornflowerBlue, plUInt32 uiRenderTargetClearMask = 0xFFFFFFFF, plRectFloat* pViewport = nullptr, plRectU32* pScissor = nullptr);

  plGALRenderCommandEncoder* BeginRendering(plColor clearColor, plUInt32 uiRenderTargetClearMask, plRectFloat* pViewport = nullptr, plRectU32* pScissor = nullptr);
  void EndRendering();

  void RenderCube(plRectFloat viewport, plMat4 mMVP, plUInt32 uiRenderTargetClearMask, plGALResourceViewHandle hSRV);

  void MostBasicTriangleTest();
  void ViewportScissorTest();
  void VertexBufferTest();
  void IndexBufferTest();
  void ConstantBufferTest();
  void StructuredBufferTest();
  void Texture2D();
  void Texture2DArray();
  void GenerateMipMaps();
  void Timestamps();

private:
  plInt32 m_iFrame = 0;
  bool m_bCaptureImage = false;
  plHybridArray<plUInt32, 8> m_ImgCompFrames;

  plShaderResourceHandle m_hMostBasicTriangleShader;
  plShaderResourceHandle m_hNDCPositionOnlyShader;
  plShaderResourceHandle m_hConstantBufferShader;
  plShaderResourceHandle m_hInstancingShader;

  plMeshBufferResourceHandle m_hTriangleMesh;
  plMeshBufferResourceHandle m_hSphereMesh;

  plConstantBufferStorageHandle m_hTestColorsConstantBuffer;
  plConstantBufferStorageHandle m_hTestPositionsConstantBuffer;

  plGALBufferHandle m_hInstancingData;
  plGALResourceViewHandle m_hInstancingDataView_8_4;
  plGALResourceViewHandle m_hInstancingDataView_12_4;

  plGALTextureHandle m_hTexture2D;
  plGALResourceViewHandle m_hTexture2D_Mip0;
  plGALResourceViewHandle m_hTexture2D_Mip1;
  plGALResourceViewHandle m_hTexture2D_Mip2;
  plGALResourceViewHandle m_hTexture2D_Mip3;
  plGALTextureHandle m_hTexture2DArray;
  plGALResourceViewHandle m_hTexture2DArray_Layer0_Mip0;
  plGALResourceViewHandle m_hTexture2DArray_Layer0_Mip1;
  plGALResourceViewHandle m_hTexture2DArray_Layer1_Mip0;
  plGALResourceViewHandle m_hTexture2DArray_Layer1_Mip1;
  plMeshBufferResourceHandle m_hCubeUV;

  bool m_bTimestampsValid = false;
  plTime m_CPUTime[2];
  plTime m_GPUTime[2];
  plGALTimestampHandle m_timestamps[2];
};
