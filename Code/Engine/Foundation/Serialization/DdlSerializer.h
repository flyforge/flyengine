#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/UniquePtr.h>

class plOpenDdlReaderElement;

struct PLASMA_FOUNDATION_DLL plSerializedBlock
{
  plString m_Name;
  plUniquePtr<plAbstractObjectGraph> m_Graph;
};

class PLASMA_FOUNDATION_DLL plAbstractGraphDdlSerializer
{
public:
  static void Write(plStreamWriter& inout_stream, const plAbstractObjectGraph* pGraph, const plAbstractObjectGraph* pTypesGraph = nullptr, bool bCompactMmode = true, plOpenDdlWriter::TypeStringMode typeMode = plOpenDdlWriter::TypeStringMode::Shortest);
  static plResult Read(plStreamReader& inout_stream, plAbstractObjectGraph* pGraph, plAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);

  static void Write(plOpenDdlWriter& inout_stream, const plAbstractObjectGraph* pGraph, const plAbstractObjectGraph* pTypesGraph = nullptr);
  static plResult Read(const plOpenDdlReaderElement* pRootElement, plAbstractObjectGraph* pGraph, plAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);

  static void WriteDocument(plStreamWriter& inout_stream, const plAbstractObjectGraph* pHeader, const plAbstractObjectGraph* pGraph, const plAbstractObjectGraph* pTypes, bool bCompactMode = true, plOpenDdlWriter::TypeStringMode typeMode = plOpenDdlWriter::TypeStringMode::Shortest);
  static plResult ReadDocument(plStreamReader& inout_stream, plUniquePtr<plAbstractObjectGraph>& ref_pHeader, plUniquePtr<plAbstractObjectGraph>& ref_pGraph, plUniquePtr<plAbstractObjectGraph>& ref_pTypes, bool bApplyPatches = true);

  static plResult ReadHeader(plStreamReader& inout_stream, plAbstractObjectGraph* pGraph);

private:
  static plResult ReadBlocks(plStreamReader& stream, plHybridArray<plSerializedBlock, 3>& blocks);
};
