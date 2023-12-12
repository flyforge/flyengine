#include <CoreTest/CoreTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>

PLASMA_CREATE_SIMPLE_TEST(World, Camera)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "LookAt")
  {
    plCamera camera;

    camera.LookAt(plVec3(0, 0, 0), plVec3(1, 0, 0), plVec3(0, 0, 1));
    PLASMA_TEST_VEC3(camera.GetPosition(), plVec3(0, 0, 0), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirForwards(), plVec3(1, 0, 0), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirRight(), plVec3(0, 1, 0), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirUp(), plVec3(0, 0, 1), plMath::DefaultEpsilon<float>());

    camera.LookAt(plVec3(0, 0, 0), plVec3(-1, 0, 0), plVec3(0, 0, 1));
    PLASMA_TEST_VEC3(camera.GetPosition(), plVec3(0, 0, 0), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirForwards(), plVec3(-1, 0, 0), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirRight(), plVec3(0, -1, 0), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirUp(), plVec3(0, 0, 1), plMath::DefaultEpsilon<float>());

    camera.LookAt(plVec3(0, 0, 0), plVec3(0, 0, 1), plVec3(0, 1, 0));
    PLASMA_TEST_VEC3(camera.GetPosition(), plVec3(0, 0, 0), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirForwards(), plVec3(0, 0, 1), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirRight(), plVec3(1, 0, 0), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirUp(), plVec3(0, 1, 0), plMath::DefaultEpsilon<float>());

    camera.LookAt(plVec3(0, 0, 0), plVec3(0, 0, -1), plVec3(0, 1, 0));
    PLASMA_TEST_VEC3(camera.GetPosition(), plVec3(0, 0, 0), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirForwards(), plVec3(0, 0, -1), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirRight(), plVec3(-1, 0, 0), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirUp(), plVec3(0, 1, 0), plMath::DefaultEpsilon<float>());

    const plMat4 mLookAt = plGraphicsUtils::CreateLookAtViewMatrix(plVec3(2, 3, 4), plVec3(3, 3, 4), plVec3(0, 0, 1), plHandedness::LeftHanded);
    camera.SetViewMatrix(mLookAt);

    PLASMA_TEST_VEC3(camera.GetPosition(), plVec3(2, 3, 4), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirForwards(), plVec3(1, 0, 0), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirRight(), plVec3(0, 1, 0), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirUp(), plVec3(0, 0, 1), plMath::DefaultEpsilon<float>());

    // look at with dir == up vector
    camera.LookAt(plVec3(2, 3, 4), plVec3(2, 3, 5), plVec3(0, 0, 1));
    PLASMA_TEST_VEC3(camera.GetPosition(), plVec3(2, 3, 4), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirForwards(), plVec3(0, 0, 1), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirRight(), plVec3(0, 1, 0), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirUp(), plVec3(-1, 0, 0), plMath::DefaultEpsilon<float>());

    camera.LookAt(plVec3(2, 3, 4), plVec3(2, 3, 3), plVec3(0, 0, 1));
    PLASMA_TEST_VEC3(camera.GetPosition(), plVec3(2, 3, 4), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirForwards(), plVec3(0, 0, -1), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirRight(), plVec3(0, 1, 0), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(camera.GetDirUp(), plVec3(1, 0, 0), plMath::DefaultEpsilon<float>());
  }
}
