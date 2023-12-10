#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <TestFramework/Framework/Declarations.h>

struct plTestConfiguration;
class plImage;

class PLASMA_TEST_DLL plTestBaseClass : public plEnumerable<plTestBaseClass>
{
  friend class plTestFramework;

  PLASMA_DECLARE_ENUMERABLE_CLASS(plTestBaseClass);

public:
  // *** Override these functions to implement the required test functionality ***

  /// Override this function to give the test a proper name.
  virtual const char* GetTestName() const /*override*/ = 0;

  const char* GetSubTestName(plInt32 iIdentifier) const;

  /// Override this function to add additional information to the test configuration
  virtual void UpdateConfiguration(plTestConfiguration& ref_config) const /*override*/;

  /// \brief Implement this to add support for image comparisons. See PLASMA_TEST_IMAGE_MSG.
  virtual plResult GetImage(plImage& ref_img) { return PLASMA_FAILURE; }

  /// \brief Implement this to add support for depth buffer image comparisons. See PLASMA_TEST_DEPTH_IMAGE_MSG.
  virtual plResult GetDepthImage(plImage& ref_img) { return PLASMA_FAILURE; }

  /// \brief Used to map the 'number' for an image comparison, to a string used for finding the comparison image.
  ///
  /// By default image comparison screenshots are called 'TestName_SubTestName_XYZ'
  /// This can be fully overridden to use any other file name.
  /// The location of the comparison images (ie the folder) cannot be specified at the moment.
  virtual void MapImageNumberToString(const char* szTestName, const char* szSubTestName, plUInt32 uiImageNumber, plStringBuilder& out_sString) const;

protected:
  /// Called at startup to determine if the test can be run. Should return a detailed error message on failure.
  virtual std::string IsTestAvailable() const { return {}; };
  /// Called at startup to setup all tests. Should use 'AddSubTest' to register all the sub-tests to the test framework.
  virtual void SetupSubTests() = 0;
  /// Called to run the test that was registered with the given identifier.
  virtual plTestAppRun RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount) = 0;

  // *** Override these functions to implement optional (de-)initialization ***

  /// Called to initialize the whole test.
  virtual plResult InitializeTest() { return PLASMA_SUCCESS; }
  /// Called to deinitialize the whole test.
  virtual plResult DeInitializeTest() { return PLASMA_SUCCESS; }
  /// Called before running a sub-test to do additional initialization specifically for that test.
  virtual plResult InitializeSubTest(plInt32 iIdentifier) { return PLASMA_SUCCESS; }
  /// Called after running a sub-test to do additional deinitialization specifically for that test.
  virtual plResult DeInitializeSubTest(plInt32 iIdentifier) { return PLASMA_SUCCESS; }


  /// Adds a sub-test to the test suite. The index is used to identify it when running the sub-tests.
  void AddSubTest(const char* szName, plInt32 iIdentifier);

private:
  struct TestEntry
  {
    const char* m_szName = "";
    plInt32 m_iIdentifier = -1;
  };

  /// Removes all sub-tests.
  void ClearSubTests();

  // Called by plTestFramework.
  plResult DoTestInitialization();
  void DoTestDeInitialization();
  plResult DoSubTestInitialization(plInt32 iIdentifier);
  void DoSubTestDeInitialization(plInt32 iIdentifier);
  plTestAppRun DoSubTestRun(plInt32 iIdentifier, double& fDuration, plUInt32 uiInvocationCount);


  std::deque<TestEntry> m_Entries;
};

#define PLASMA_CREATE_TEST(TestClass)
