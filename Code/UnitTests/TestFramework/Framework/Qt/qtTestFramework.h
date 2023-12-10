#pragma once

#ifdef PLASMA_USE_QT

#  include <QObject>
#  include <TestFramework/Framework/TestFramework.h>
#  include <TestFramework/TestFrameworkDLL.h>

/// \brief Derived plTestFramework which signals the GUI to update whenever a new tests result comes in.
class PLASMA_TEST_DLL plQtTestFramework : public QObject, public plTestFramework
{
  Q_OBJECT
public:
  plQtTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int iArgc, const char** pArgv);
  virtual ~plQtTestFramework();

private:
  plQtTestFramework(plQtTestFramework&);
  void operator=(plQtTestFramework&);

Q_SIGNALS:
  void TestResultReceived(qint32 testIndex, qint32 subTestIndex);

protected:
  virtual void OutputImpl(plTestOutput::Enum Type, const char* szMsg) override;
  virtual void TestResultImpl(plInt32 iSubTestIndex, bool bSuccess, double fDuration) override;
};

#endif

