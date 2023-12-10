#include <AlphaComp/AlphaCompPCH.h>

#include <AlphaComp/AlphaComp.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/plTexFormat/plTexFormat.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Texture/Image/Image.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/IO/MemoryStream.h>

//#define DO_SANITY_CHECKS

plCommandLineOptionPath opt_InputFile("_AlphaComp", "-in", "Input path", "", "");
plCommandLineOptionPath opt_OutputFile("_AlphaComp", "-out", "OutputPath", "", "");

plAlphaComp::plAlphaComp()
  : plApplication("TexConv")
{
}

plResult plAlphaComp::BeforeCoreSystemsStartup()
{
  plStartup::AddApplicationTag("tool");
  plStartup::AddApplicationTag("texconv");

  return SUPER::BeforeCoreSystemsStartup();
}

void plAlphaComp::AfterCoreSystemsStartup()
{
  plFileSystem::AddDataDirectory("", "App", ":", plFileSystem::AllowWrites).IgnoreResult();

  plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
  plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);
}

void plAlphaComp::BeforeCoreSystemsShutdown()
{
  plGlobalLog::RemoveLogWriter(plLogWriter::Console::LogMessageHandler);
  plGlobalLog::RemoveLogWriter(plLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

struct BitWriter
{
  plDynamicArray<plUInt32> m_data;
  plUInt32 m_bitsUnused = 0;

  void Write(plUInt32 value, plUInt32 bits)
  {
    PLASMA_ASSERT_DEBUG(bits == 1, "currently only one bit is supported");
    if(m_bitsUnused == 0)
    {
      m_data.PushBack(0);
      m_bitsUnused = 32;
    }

    value = value << (32 - m_bitsUnused);
    m_data.PeekBack() |= value;
    m_bitsUnused -= 1;

    /*PLASMA_ASSERT_DEBUG(bits >= 0, "Need to write at least one bit");
    plUInt32 numAdditionalBytesRequired = (m_bitsUnused >= bits) ? 0 : (bits - m_bitsUnused - 1) / 8 + 1;
    plUInt32 startingSize = m_data.GetCount();
    if (numAdditionalBytesRequired > 0)
    {
      m_data.SetCount(startingSize + numAdditionalBytesRequired);
    }
    if(m_bitsUnused == 0)
    {
      plUInt32 numBytesToCopy = (bits - 1) / 8 + 1;
      memcpy(m_data.GetData() + startingSize, &value, numBytesToCopy);
      m_bitsUnused = 8 - (bits % 8);
    }
    else
    {
      plUInt8* outPtr = m_data.GetData() + (startingSize - 1);
      while(bits > 0)
      {
        if(m_bitsUnused == 0)
        {
          m_bitsUnused = 8;
        }
        plUInt8 valueToAdd = ((1 << m_bitsUnused) - 1) & value;
        *outPtr |= (valueToAdd << (m_bitsUnused - 1));
        value = value >> m_bitsUnused;
        outPtr++;
        if(m_bitsUnused > bits)
        {
          m_bitsUnused -= bits;
          bits = 0;
        }
        else
        {
          bits -= m_bitsUnused;
          m_bitsUnused = 0;
        }
      }
    }*/
  }
};

plApplication::Execution plAlphaComp::Run()
{
  SetReturnCode(-1);

  plString input = opt_InputFile.GetOptionValue(plCommandLineOption::LogMode::Always);
  plString output = opt_OutputFile.GetOptionValue(plCommandLineOption::LogMode::Always);
  plStringBuilder binFile = output;
  binFile.ChangeFileExtension("bin");


  plImage inputImage;
  if(inputImage.LoadFrom(input.GetData()).Failed())
  {
    plLog::Error("Failed to load input image form {}", input);
    SetReturnCode(-1);
    return plApplication::Execution::Quit;
  }

  /*plImageHeader debugHeader;
  debugHeader.SetWidth(471);
  debugHeader.SetHeight(471);
  debugHeader.SetImageFormat(plImageFormat::R32_FLOAT);
  inputImage.ResetAndAlloc(debugHeader);
  float* debugValue = inputImage.GetPixelPointer<float>(0, 0, 0, 0, 0, 0);
  for(plUInt32 y =0; y < debugHeader.GetHeight(); ++y)
  {
    for(plUInt32 x = 0; x < debugHeader.GetWidth(); ++x)
    {
      //if((x >= 32 && y < 32) || (x < 32 && y >= 32))
      //{
      //  *debugValue = 1.0f;
      //}
      float normalizedX = (static_cast<float>(x) / debugHeader.GetWidth()) * 2.0f - 1.0f;
      float normalizedY = (static_cast<float>(y) / debugHeader.GetHeight()) * 2.0f - 1.0f;
      if(plMath::Sqrt(normalizedX * normalizedX + normalizedY * normalizedY) < 1.0f)
      {
        *debugValue = 1.0f;
      }
      ++debugValue;
    }
  }
  plStringBuilder debugImg = input;
  debugImg.RemoveFileExtension();
  debugImg.Append("_debug.dds");
  inputImage.SaveTo(debugImg.GetData()).IgnoreResult();*/

  plImageHeader inputHeader = inputImage.GetHeader();

  /*
  // TO RGB png image
  plImageHeader outputHeader = inputImage.GetHeader();
  outputHeader.SetImageFormat(plImageFormat::R8G8B8_UNORM);
  outputHeader.SetWidth(inputHeader.GetWidth() / 3);
  plImage outputImage;
  outputImage.ResetAndAlloc(outputHeader);
  for(plUInt32 y = 0; y < inputHeader.GetHeight(); ++y)
  {
    const float* inputRow = inputImage.GetPixelPointer<float>(0, 0, 0, 0, y, 0);
    plUInt8* outputRow = outputImage.GetPixelPointer<plUInt8>(0, 0, 0, 0, y, 0);
    for(plUInt32 x=0; x < inputHeader.GetWidth(); ++x, ++inputRow, ++outputRow)
    {
      if(*inputRow > 0.0f)
      {
        *outputRow = 0x00;
      }
      else
      {
        *outputRow = 0xFF;
      }
    }
  }
  if(outputImage.SaveTo(output.GetData()).Failed())
  {
    plLog::Error("Failed to write output image to {}", output);
    SetReturnCode(-1);
    return plApplication::Execution::Quit;
  }*/

  constexpr plUInt32 blockSize = 32;

  plStopwatch watch;

  plUInt32 numFullBlocksX = inputHeader.GetWidth() / blockSize;
  plUInt32 numFullBlocksY = inputHeader.GetHeight() / blockSize;
  bool partialBlockX = (inputHeader.GetWidth() % blockSize) != 0;
  bool partialBlockY = (inputHeader.GetHeight() % blockSize) != 0;

  BitWriter writer;

#ifdef DO_SANITY_CHECKS
  plUInt32 numOnPixels = 0;
#endif
  for (plUInt32 blockY = 0; blockY < numFullBlocksY; blockY++)
  {
    for (plUInt32 blockX = 0; blockX < numFullBlocksX; blockX++)
    {
      const plUInt32 blockStartX = blockX * blockSize;
      const plUInt32 blockStartY = blockY * blockSize;
      bool blockHasEdge = false;
      for(plUInt32 y=0; y < blockSize; y++)
      {
        const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
        for(plUInt32 x=0; x < blockSize; ++x, ++inputValue)
        {
          if(*inputValue > 0.0f)
          {
            blockHasEdge = true;
            goto hasEdgeEnd;
          }
        }
      }
    hasEdgeEnd:
      writer.Write(blockHasEdge ? 1u : 0u, 1);

      if(blockHasEdge)
      {
        for (plUInt32 y = 0; y < blockSize; y++)
        {
          const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
          for (plUInt32 x = 0; x < blockSize; ++x, ++inputValue)
          {
            writer.Write(*inputValue > 0.0f ? 1u : 0u, 1);
#ifdef DO_SANITY_CHECKS
            if(*inputValue > 0.0f)
            {
              numOnPixels += 1;
            }
#endif
          }
        }
      }
    }
  }

  if(partialBlockX)
  {
    const plUInt32 blockStartX = numFullBlocksX * blockSize;
    const plUInt32 remainingBlockSizeX = inputImage.GetWidth() - blockStartX;
    for(plUInt32 blockY = 0; blockY < numFullBlocksY; blockY++)
    {
      const plUInt32 blockStartY = blockY * blockSize;
      bool blockHasEdge = false;
      for (plUInt32 y = 0; y < blockSize; y++)
      {
        const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
        for (plUInt32 x = 0; x < remainingBlockSizeX; ++x, ++inputValue)
        {
          if (*inputValue > 0.0f)
          {
            blockHasEdge = true;
            goto hasEdgeEnd2;
          }
        }
      }
    hasEdgeEnd2:
      writer.Write(blockHasEdge ? 1u : 0u, 1);

      if (blockHasEdge)
      {
        for (plUInt32 y = 0; y < blockSize; y++)
        {
          const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
          for (plUInt32 x = 0; x < remainingBlockSizeX; ++x, ++inputValue)
          {
            writer.Write(*inputValue > 0.0f ? 1u : 0u, 1);
#ifdef DO_SANITY_CHECKS
            if (*inputValue > 0.0f)
            {
              numOnPixels += 1;
            }
#endif
          }
        }
      }
    }
  }

  if (partialBlockY)
  {
    const plUInt32 blockStartY = numFullBlocksY * blockSize;
    const plUInt32 remainingBlockSizeY = inputImage.GetHeight() - blockStartY;
    for (plUInt32 blockX = 0; blockX < numFullBlocksX; blockX++)
    {
      const plUInt32 blockStartX = blockX * blockSize;
      bool blockHasEdge = false;
      for (plUInt32 y = 0; y < remainingBlockSizeY; y++)
      {
        const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
        for (plUInt32 x = 0; x < blockSize; ++x, ++inputValue)
        {
          if (*inputValue > 0.0f)
          {
            blockHasEdge = true;
            goto hasEdgeEnd3;
          }
        }
      }
    hasEdgeEnd3:
      writer.Write(blockHasEdge ? 1u : 0u, 1);

      if (blockHasEdge)
      {
        for (plUInt32 y = 0; y < remainingBlockSizeY; y++)
        {
          const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
          for (plUInt32 x = 0; x < blockSize; ++x, ++inputValue)
          {
            writer.Write(*inputValue > 0.0f ? 1u : 0u, 1);
#ifdef DO_SANITY_CHECKS
            if (*inputValue > 0.0f)
            {
              numOnPixels += 1;
            }
#endif
          }
        }
      }
    }
  }

  if(partialBlockX && partialBlockY)
  {
    const plUInt32 blockStartX = numFullBlocksX * blockSize;
    const plUInt32 remainingBlockSizeX = inputImage.GetWidth() - blockStartX;
    const plUInt32 blockStartY = numFullBlocksY * blockSize;
    const plUInt32 remainingBlockSizeY = inputImage.GetHeight() - blockStartY;

    bool blockHasEdge = false;
    for (plUInt32 y = 0; y < remainingBlockSizeY; y++)
    {
      const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
      for (plUInt32 x = 0; x < remainingBlockSizeX; ++x, ++inputValue)
      {
        if (*inputValue > 0.0f)
        {
          blockHasEdge = true;
          goto hasEdgeEnd4;
        }
      }
    }
  hasEdgeEnd4:
    writer.Write(blockHasEdge ? 1u : 0u, 1);

    if (blockHasEdge)
    {
      for (plUInt32 y = 0; y < remainingBlockSizeY; y++)
      {
        const float* inputValue = inputImage.GetPixelPointer<float>(0, 0, 0, blockStartX, blockStartY + y, 0);
        for (plUInt32 x = 0; x < remainingBlockSizeX; ++x, ++inputValue)
        {
          writer.Write(*inputValue > 0.0f ? 1u : 0u, 1);
#ifdef DO_SANITY_CHECKS
          if (*inputValue > 0.0f)
          {
            numOnPixels += 1;
          }
#endif
        }
      }
    }
  }

  auto step1 = watch.GetRunningTotal();

#ifdef DO_SANITY_CHECKS
  plUInt32 numOnBits = 0;
  for(auto& b : writer.m_data)
  {
    for(plUInt8 v = 1, x = 0; x < 8; x++, v <<= 1)
    {
      if(b & v)
      {
        numOnBits += 1;
      }
    }
  }

  plLog::Info("NumOnPixels {} NumOnBits {}", numOnPixels, numOnBits);
  if(numOnBits < numOnPixels)
  {
    plLog::Error("numOnBits is incorrect!");
  }
#endif

  plDynamicArray<plUInt8> compressedMemory;
  compressedMemory.SetCountUninitialized(writer.m_data.GetCount() * sizeof(plUInt32));
  plRawMemoryStreamWriter memoryStreamWriter(compressedMemory.GetData(), compressedMemory.GetCount());

  plCompressedStreamWriterZstd compressedWriter;
  compressedWriter.SetOutputStream(&memoryStreamWriter, plCompressedStreamWriterZstd::Default);

  if(compressedWriter.WriteBytes(writer.m_data.GetData(), writer.m_data.GetCount() * sizeof(plUInt32)).Failed())
  {
    plLog::Error("Failed to write {} worth of data", writer.m_data.GetCount() * sizeof(plUInt32));
    SetReturnCode(-1);
    return plApplication::Execution::Quit;
  }
  compressedWriter.FinishCompressedStream().IgnoreResult();

  auto step2 = watch.GetRunningTotal();

  plLog::Info("Pre zstd size {}", writer.m_data.GetCount() * sizeof(plUInt32));
  plLog::Info("Post zstd size {}", memoryStreamWriter.GetNumWrittenBytes());

  plLog::Info("Time for compression {} ({} {})", plArgF(step1.GetMilliseconds() + step2.GetMilliseconds(), 4), plArgF(step1.GetMilliseconds(), 4), plArgF(step2.GetMilliseconds(), 4));

  plFileWriter fileWriter;

  if (fileWriter.Open(binFile.GetData()).Failed())
  {
    plLog::Error("Failed to open output file {}", output);
    SetReturnCode(-1);
    return plApplication::Execution::Quit;
  }
  PLASMA_ASSERT_DEV(compressedMemory.GetCount() >= memoryStreamWriter.GetNumWrittenBytes(), "out of bounds write");
  fileWriter.WriteBytes(compressedMemory.GetData(), memoryStreamWriter.GetNumWrittenBytes()).IgnoreResult();
  fileWriter.Close();

  plDynamicArray<plUInt8> uncompressedMemory;
  uncompressedMemory.SetCountUninitialized(writer.m_data.GetCount() * sizeof(plUInt32));

  watch.StopAndReset();
  watch.Resume();

  plRawMemoryStreamReader rawReader(compressedMemory.GetData(), memoryStreamWriter.GetNumWrittenBytes());
  plCompressedStreamReaderZstd decompressor(&rawReader);
  auto bytesRead = decompressor.ReadBytes(uncompressedMemory.GetData(), uncompressedMemory.GetCount());
  PLASMA_ASSERT_DEV(bytesRead == uncompressedMemory.GetCount(), "decompression failed");

  plUInt32* pCur = (plUInt32*)uncompressedMemory.GetData();
  const plUInt32* pStart = pCur;
  const plUInt32* pEnd = (plUInt32*)(uncompressedMemory.GetData() + uncompressedMemory.GetCount());
  plUInt32 curValue = 0;
  plUInt32 bitsLeft = 0;

  plDynamicArray<plUInt32> decompressedImage;
  plUInt32 decompressedImageRowPitch = (inputHeader.GetWidth() - 1) / 32 + 1;
  decompressedImage.SetCount(decompressedImageRowPitch * inputHeader.GetHeight());

  for(plUInt32 blockY = 0;blockY < numFullBlocksY; blockY++)
  {
    for(plUInt32 blockX = 0; blockX < numFullBlocksX; blockX++)
    {
      if (bitsLeft == 0)
      {
        bitsLeft = 32;
        curValue = *pCur;
        ++pCur;
      }
      bool blockHasEdge = curValue & 1;
      bitsLeft--;
      curValue >>= 1;
      if(blockHasEdge)
      {
        if(bitsLeft == 32)
        {
          for (plUInt32 y = 0; y < blockSize; y++)
          {
            plUInt32* dst = decompressedImage.GetData() + (blockY * blockSize + y) * decompressedImageRowPitch + blockX * (blockSize / 32);
            for (plUInt32 x = 0; x < blockSize; x += 32, ++dst, ++pCur)
            {
              *dst = curValue;
              curValue = *pCur;
            }
          }
        }
        else
        {
          for (plUInt32 y = 0; y < blockSize; y++)
          {
            plUInt32* dst = decompressedImage.GetData() + (blockY * blockSize + y) * decompressedImageRowPitch + blockX * (blockSize / 32);
            for (plUInt32 x = 0; x < blockSize; x += 32, ++dst, ++pCur)
            {
              plUInt32 nextValue = *pCur;
              *dst = curValue | (nextValue << bitsLeft);
              curValue = nextValue >> (32 - bitsLeft);
            }
          }
        }
      }
    }
  }

  if (partialBlockX)
  {
    const plUInt32 blockStartX = numFullBlocksX * blockSize;
    const plUInt32 remainingBlockSizeX = inputImage.GetWidth() - blockStartX;
    const plUInt32 tailReads = remainingBlockSizeX % 32;
    const plUInt32 remainingFullReads = remainingBlockSizeX - tailReads;

    for (plUInt32 blockY = 0; blockY < numFullBlocksY; blockY++)
    {
      if (bitsLeft == 0)
      {
        bitsLeft = 32;
        curValue = *pCur;
        ++pCur;
      }
      bool blockHasEdge = curValue & 1;
      bitsLeft--;
      curValue >>= 1;
      if (blockHasEdge)
      {
        for (plUInt32 y = 0; y < blockSize; y++)
        {
          if(bitsLeft == 0)
          {
            curValue = *pCur;
            ++pCur;
            bitsLeft = 32;
          }
          plUInt32* dst = decompressedImage.GetData() + (blockY * blockSize + y) * decompressedImageRowPitch + numFullBlocksX * (blockSize / 32);
          if(bitsLeft == 32)
          {
            for (plUInt32 x = 0; x < remainingFullReads; x += 32, ++dst, ++pCur)
            {
              *dst = curValue;
              curValue = *pCur;
            }
          }
          else
          {
            for (plUInt32 x = 0; x < remainingFullReads; x += 32, ++dst, ++pCur)
            {
              plUInt32 nextValue = *pCur;
              *dst = curValue | (nextValue << bitsLeft);
              curValue = nextValue >> (32 - bitsLeft);
            }
          }
          if (tailReads > 0)
          {
            if (tailReads > bitsLeft)
            {
              plUInt32 nextValue = *pCur;
              plUInt32 nextMask = ((1 << (tailReads - bitsLeft)) - 1);
              *dst = curValue | ((nextValue & nextMask) << bitsLeft);
              curValue = nextValue >> (tailReads - bitsLeft);
              bitsLeft = bitsLeft + 32 - tailReads;
              ++pCur;
            }
            else
            {
              *dst = curValue & ((1 << tailReads) - 1);
              curValue >>= tailReads;
              bitsLeft -= tailReads;
            }
            ++dst;
          }
        }
      }
    }
  }

  if (partialBlockY)
  {
    const plUInt32 blockStartY = numFullBlocksY * blockSize;
    const plUInt32 remainingBlockSizeY = inputImage.GetHeight() - blockStartY;

    for (plUInt32 blockX = 0; blockX < numFullBlocksX; blockX++)
    {
      if (bitsLeft == 0)
      {
        bitsLeft = 32;
        curValue = *pCur;
        ++pCur;
      }
      bool blockHasEdge = curValue & 1;
      bitsLeft--;
      curValue >>= 1;
      if (blockHasEdge)
      {
        if(bitsLeft == 32)
        {
          for (plUInt32 y = 0; y < remainingBlockSizeY; y++)
          {
            plUInt32* dst = decompressedImage.GetData() + (numFullBlocksY * blockSize + y) * decompressedImageRowPitch + blockX * (blockSize / 32);
            for (plUInt32 x = 0; x < blockSize; x += 32, ++dst, ++pCur)
            {
              *dst = curValue;
              curValue = *pCur;
            }
          }
        }
        else
        {
          for (plUInt32 y = 0; y < remainingBlockSizeY; y++)
          {
            plUInt32* dst = decompressedImage.GetData() + (numFullBlocksY * blockSize + y) * decompressedImageRowPitch + blockX * (blockSize / 32);
            for (plUInt32 x = 0; x < blockSize; x += 32, ++dst, ++pCur)
            {
              plUInt32 nextValue = *pCur;
              *dst = curValue | (nextValue << bitsLeft);
              curValue = nextValue >> (32 - bitsLeft);
            }
          }
        }
      }
    }
  }

  if(partialBlockX && partialBlockY)
  {
    const plUInt32 blockStartX = numFullBlocksX * blockSize;
    const plUInt32 remainingBlockSizeX = inputImage.GetWidth() - blockStartX;
    const plUInt32 tailReadsX = remainingBlockSizeX % 32;
    const plUInt32 remainingFullReadsX = remainingBlockSizeX - tailReadsX;

    const plUInt32 blockStartY = numFullBlocksY * blockSize;
    const plUInt32 remainingBlockSizeY = inputImage.GetHeight() - blockStartY;

    if (bitsLeft == 0)
    {
      bitsLeft = 32;
      curValue = *pCur;
      ++pCur;
    }
    bool blockHasEdge = curValue & 1;
    bitsLeft--;
    curValue >>= 1;
    if (blockHasEdge)
    {
      for (plUInt32 y = 0; y < remainingBlockSizeY; y++)
      {
        if (bitsLeft == 0)
        {
          curValue = *pCur;
          ++pCur;
          bitsLeft = 32;
        }
        plUInt32* dst = decompressedImage.GetData() + (blockStartY + y) * decompressedImageRowPitch + numFullBlocksX * (blockSize / 32);
        if(bitsLeft == 32)
        {
          for (plUInt32 x = 0; x < remainingFullReadsX; x += 32, ++dst, ++pCur)
          {
            *dst = curValue;
            curValue = *pCur;
          }
        }
        else
        {
          for (plUInt32 x = 0; x < remainingFullReadsX; x += 32, ++dst, ++pCur)
          {
            plUInt32 nextValue = *pCur;
            *dst = curValue | (nextValue << bitsLeft);
            curValue = nextValue >> (32 - bitsLeft);
          }
        }
        if (tailReadsX > 0)
        {
          if (tailReadsX > bitsLeft)
          {
            plUInt32 nextValue = *pCur;
            plUInt32 nextMask = ((1 << (tailReadsX - bitsLeft)) - 1);
            *dst = curValue | ((nextValue & nextMask) << bitsLeft);
            curValue = nextValue >> (tailReadsX - bitsLeft);
            bitsLeft = bitsLeft + 32 - tailReadsX;
            ++pCur;
          }
          else
          {
            *dst = curValue & ((1 << tailReadsX) - 1);
            curValue >>= tailReadsX;
            bitsLeft -= tailReadsX;
          }
          ++dst;
        }
      }
    }
  }

  auto decompressEnd = watch.GetRunningTotal();
  plLog::Info("Decompression took {}", plArgF(decompressEnd.GetMilliseconds(), 4));

  plImageHeader testHeader = inputHeader;
  testHeader.SetImageFormat(plImageFormat::R8G8B8_UNORM);

  plImage testImage;
  testImage.ResetAndAlloc(testHeader);

  for(plUInt32 y=0; y < testHeader.GetHeight(); ++y)
  {
    const plUInt32* src = decompressedImage.GetData() + (y * decompressedImageRowPitch);
    plUInt8* outputRow = testImage.GetPixelPointer<plUInt8>(0, 0, 0, 0, y, 0);
    for(plUInt32 x=0; x < testHeader.GetWidth(); ++x, outputRow += 3)
    {
      plUInt32 value = src[x / 32];
      if (value & (1 << (x % 32)))
      {
        outputRow[0] = 0xFF;
        outputRow[1] = 0xFF;
        outputRow[2] = 0xFF;
      }
      else
      {
        outputRow[0] = 0x0;
        outputRow[1] = 0x0;
        outputRow[2] = 0x0;
      }
    }
  }

  testImage.SaveTo(output.GetData()).IgnoreResult();

  SetReturnCode(0);
  return plApplication::Execution::Quit;
}

PLASMA_CONSOLEAPP_ENTRY_POINT(plAlphaComp);