#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class PL_FOUNDATION_DLL plAbstractGraphBinarySerializer
{
public:
  static void Write(plStreamWriter& inout_stream, const plAbstractObjectGraph* pGraph, const plAbstractObjectGraph* pTypesGraph = nullptr);                // [tested]
  static void Read(plStreamReader& inout_stream, plAbstractObjectGraph* pGraph, plAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = false); // [tested]

private:
};
