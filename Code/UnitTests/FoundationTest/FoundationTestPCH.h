#pragma once

#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/ConstructionCounter.h>

#include <Foundation/Basics.h>
#include <Foundation/Basics/Assert.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/TypeTraits.h>
#include <Foundation/Types/Types.h>
#include <Foundation/Types/Variant.h>

#include <Foundation/Math/Declarations.h>

using plMathTestType = float;

using plVec2T = plVec2Template<plMathTestType>;                           ///< This is only for testing purposes
using plVec3T = plVec3Template<plMathTestType>;                           ///< This is only for testing purposes
using plVec4T = plVec4Template<plMathTestType>;                           ///< This is only for testing purposes
using plMat3T = plMat3Template<plMathTestType>;                           ///< This is only for testing purposes
using plMat4T = plMat4Template<plMathTestType>;                           ///< This is only for testing purposes
using plQuatT = plQuatTemplate<plMathTestType>;                           ///< This is only for testing purposes
using plPlaneT = plPlaneTemplate<plMathTestType>;                         ///< This is only for testing purposes
using plBoundingBoxT = plBoundingBoxTemplate<plMathTestType>;             ///< This is only for testing purposes
using plBoundingBoxSphereT = plBoundingBoxSphereTemplate<plMathTestType>; ///< This is only for testing purposes
using plBoundingSphereT = plBoundingSphereTemplate<plMathTestType>;       ///< This is only for testing purposes
using plTransformT = plTransformTemplate<plMathTestType>;

#define plasmaFoundationTest_Plugin1 "plasmaFoundationTest_Plugin1"
#define plasmaFoundationTest_Plugin2 "plasmaFoundationTest_Plugin2"
