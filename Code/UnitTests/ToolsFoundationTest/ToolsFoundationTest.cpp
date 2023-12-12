#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

plInt32 plConstructionCounter::s_iConstructions = 0;
plInt32 plConstructionCounter::s_iDestructions = 0;
plInt32 plConstructionCounter::s_iConstructionsLast = 0;
plInt32 plConstructionCounter::s_iDestructionsLast = 0;

PLASMA_TESTFRAMEWORK_ENTRY_POINT("ToolsFoundationTest", "Tools Foundation Tests")
