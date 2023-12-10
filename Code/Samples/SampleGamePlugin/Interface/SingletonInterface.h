#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Strings/FormatString.h>

// BEGIN-DOCS-CODE-SNIPPET: singleton-interface
/// \brief Pure virtual interface for demonstrating the singleton work flow
///
/// This declaration would typically be in a shared location, that all code can #include
class PrintInterface
{
public:
  virtual ~PrintInterface() = default;

  virtual void Print(const plFormatString& text) = 0;
};
// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: singleton-impl-declaration
/// \brief Implementation of the PrintInterface, just forwards the text to plLog::Info()
///
/// This would typically be in a different plugin than the interface and would be allocated by that plugin on startup.
class PrintImplementation : public PrintInterface
{
  PLASMA_DECLARE_SINGLETON_OF_INTERFACE(PrintImplementation, PrintInterface);

public:
  PrintImplementation();

  virtual void Print(const plFormatString& text) override;

private:
  // needed for the startup system to be able to call the private function below
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(SampleGamePluginStartupGroup, SampleGamePluginMainStartup);

  void OnCoreSystemsStartup()
  {
    /* we could do something important here */
  }
};
// END-DOCS-CODE-SNIPPET
