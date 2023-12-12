#include <TestFramework/Framework/TestFramework.h>

#include <Foundation/Basics.h>
#include <Foundation/Basics/Assert.h>
#include <Foundation/Types/TypeTraits.h>
#include <Foundation/Types/Types.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>

#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

#include <Foundation/Math/Declarations.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>

using plMathTestType = float;

using plVec2T = plVec2Template<plMathTestType>;                     ///< This is only for testing purposes
using plVec3T = plVec3Template<plMathTestType>;                     ///< This is only for testing purposes
using plVec4T = plVec4Template<plMathTestType>;                     ///< This is only for testing purposes
using plMat3T = plMat3Template<plMathTestType>;                     ///< This is only for testing purposes
using plMat4T = plMat4Template<plMathTestType>;                     ///< This is only for testing purposes
using plQuatT = plQuatTemplate<plMathTestType>;                     ///< This is only for testing purposes
using plPlaneT = plPlaneTemplate<plMathTestType>;                   ///< This is only for testing purposes
using plBoundingBoxT = plBoundingBoxTemplate<plMathTestType>;       ///< This is only for testing purposes
using plBoundingSphereT = plBoundingSphereTemplate<plMathTestType>; ///< This is only for testing purposes
