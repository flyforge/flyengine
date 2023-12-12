#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Utilities/Progress.h>

PLASMA_CREATE_SIMPLE_TEST(Utility, Progress)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Simple progress")
  {
    plProgress progress;
    {
      plProgressRange progressRange = plProgressRange("TestProgress", 4, false, &progress);

      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.0f, plMath::DefaultEpsilon<float>());
      PLASMA_TEST_BOOL(progress.GetMainDisplayText() == "TestProgress");

      progressRange.BeginNextStep("Step1");
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.0f, plMath::DefaultEpsilon<float>());
      PLASMA_TEST_BOOL(progress.GetStepDisplayText() == "Step1");

      progressRange.BeginNextStep("Step2");
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.25f, plMath::DefaultEpsilon<float>());
      PLASMA_TEST_BOOL(progress.GetStepDisplayText() == "Step2");

      progressRange.BeginNextStep("Step3");
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.5f, plMath::DefaultEpsilon<float>());
      PLASMA_TEST_BOOL(progress.GetStepDisplayText() == "Step3");

      progressRange.BeginNextStep("Step4");
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.75f, plMath::DefaultEpsilon<float>());
      PLASMA_TEST_BOOL(progress.GetStepDisplayText() == "Step4");
    }

    PLASMA_TEST_FLOAT(progress.GetCompletion(), 1.0f, plMath::DefaultEpsilon<float>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Weighted progress")
  {
    plProgress progress;
    {
      plProgressRange progressRange = plProgressRange("TestProgress", 4, false, &progress);
      progressRange.SetStepWeighting(2, 2.0f);

      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.0f, plMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step0+1", 2);
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.2f, plMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step2"); // this step should have twice the weight as the other steps.
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.4f, plMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step3");
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.8f, plMath::DefaultEpsilon<float>());
    }

    PLASMA_TEST_FLOAT(progress.GetCompletion(), 1.0f, plMath::DefaultEpsilon<float>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Nested progress")
  {
    plProgress progress;
    {
      plProgressRange progressRange = plProgressRange("TestProgress", 4, false, &progress);
      progressRange.SetStepWeighting(2, 2.0f);

      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.0f, plMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step0");
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.0f, plMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step1");
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.2f, plMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step2");
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.4f, plMath::DefaultEpsilon<float>());

      {
        plProgressRange nestedRange = plProgressRange("Nested", 5, false, &progress);
        nestedRange.SetStepWeighting(1, 4.0f);

        PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.4f, plMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep0");
        PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.4f, plMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep1");
        PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.45f, plMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep2");
        PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.65f, plMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep3");
        PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.7f, plMath::DefaultEpsilon<float>());

        nestedRange.BeginNextStep("NestedStep4");
        PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.75f, plMath::DefaultEpsilon<float>());
      }
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.8f, plMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step3");
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.8f, plMath::DefaultEpsilon<float>());
    }

    PLASMA_TEST_FLOAT(progress.GetCompletion(), 1.0f, plMath::DefaultEpsilon<float>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Nested progress with manual completion")
  {
    plProgress progress;
    {
      plProgressRange progressRange = plProgressRange("TestProgress", 3, false, &progress);
      progressRange.SetStepWeighting(1, 2.0f);

      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.0f, plMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step0");
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.0f, plMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step1");
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.25f, plMath::DefaultEpsilon<float>());

      {
        plProgressRange nestedRange = plProgressRange("Nested", false, &progress);

        PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.25f, plMath::DefaultEpsilon<float>());

        nestedRange.SetCompletion(0.5);
        PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.5f, plMath::DefaultEpsilon<float>());
      }
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.75f, plMath::DefaultEpsilon<float>());

      progressRange.BeginNextStep("Step2");
      PLASMA_TEST_FLOAT(progress.GetCompletion(), 0.75f, plMath::DefaultEpsilon<float>());
    }

    PLASMA_TEST_FLOAT(progress.GetCompletion(), 1.0f, plMath::DefaultEpsilon<float>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Progress Events")
  {
    plUInt32 uiNumProgressUpdatedEvents = 0;

    plProgress progress;
    progress.m_Events.AddEventHandler([&](const plProgressEvent& e) {
      if (e.m_Type == plProgressEvent::Type::ProgressChanged)
      {
        ++uiNumProgressUpdatedEvents;
        PLASMA_TEST_FLOAT(e.m_pProgressbar->GetCompletion(), uiNumProgressUpdatedEvents * 0.25f, plMath::DefaultEpsilon<float>());
      } });

    {
      plProgressRange progressRange = plProgressRange("TestProgress", 4, false, &progress);

      progressRange.BeginNextStep("Step1");
      progressRange.BeginNextStep("Step2");
      progressRange.BeginNextStep("Step3");
      progressRange.BeginNextStep("Step4");
    }

    PLASMA_TEST_FLOAT(progress.GetCompletion(), 1.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_INT(uiNumProgressUpdatedEvents, 4);
  }
}
