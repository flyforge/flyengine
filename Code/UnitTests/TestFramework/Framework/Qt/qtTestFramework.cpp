#include <TestFramework/TestFrameworkPCH.h>

#ifdef PLASMA_USE_QT
#  include <TestFramework/Framework/Qt/qtTestFramework.h>

////////////////////////////////////////////////////////////////////////
// plQtTestFramework public functions
////////////////////////////////////////////////////////////////////////

plQtTestFramework::plQtTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int iArgc, const char** pArgv)
  : plTestFramework(szTestName, szAbsTestDir, szRelTestDataDir, iArgc, pArgv)
{
  Q_INIT_RESOURCE(resources);
  Initialize();
}

plQtTestFramework::~plQtTestFramework() = default;


////////////////////////////////////////////////////////////////////////
// plQtTestFramework protected functions
////////////////////////////////////////////////////////////////////////

void plQtTestFramework::OutputImpl(plTestOutput::Enum Type, const char* szMsg)
{
  plTestFramework::OutputImpl(Type, szMsg);
}

void plQtTestFramework::TestResultImpl(plInt32 iSubTestIndex, bool bSuccess, double fDuration)
{
  plTestFramework::TestResultImpl(iSubTestIndex, bSuccess, fDuration);
  Q_EMIT TestResultReceived(m_iCurrentTestIndex, iSubTestIndex);
}

#endif

PLASMA_STATICLINK_FILE(TestFramework, TestFramework_Framework_Qt_qtTestFramework);
