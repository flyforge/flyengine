#  include <Foundation/Math/Math.h>
#  include <Foundation/Math/Quat.h>
#  include <Foundation/Math/Vec2.h>
#  include <Foundation/Math/Vec3.h>
#  include <Foundation/Math/Vec4.h>
#  include <Foundation/Math/Transform.h>
#  include <Foundation/Math/Mat3.h>
#  include <Foundation/Math/Mat4.h>

#ifndef BUILDSYSTEM_BUILDING_DLANGCODEGENTOOL_LIB

#  include <GameEngine/Animation/RotorComponent.h>
#  include <DLangPlugin/Interop/DLangLog_inl.h>

template struct __declspec(dllexport) plVec2Template<float>;
template struct __declspec(dllexport) plVec2Template<double>;

template struct __declspec(dllexport) plVec3Template<float>;
template struct __declspec(dllexport) plVec3Template<double>;

template struct __declspec(dllexport) plVec4Template<float>;
template struct __declspec(dllexport) plVec4Template<double>;

template struct __declspec(dllexport) plQuatTemplate<float>;
template struct __declspec(dllexport) plQuatTemplate<double>;

template struct __declspec(dllexport) plMat3Template<float>;
template struct __declspec(dllexport) plMat3Template<double>;

template struct __declspec(dllexport) plMat4Template<float>;
template struct __declspec(dllexport) plMat4Template<double>;

template struct __declspec(dllexport) plTransformTemplate<float>;
template struct __declspec(dllexport) plTransformTemplate<double>;

#endif
