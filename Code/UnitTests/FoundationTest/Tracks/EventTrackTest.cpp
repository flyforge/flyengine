#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Tracks/EventTrack.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(Tracks);

PLASMA_CREATE_SIMPLE_TEST(Tracks, EventTrack)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Empty")
  {
    plEventTrack et;
    plHybridArray<plHashedString, 8> result;

    PLASMA_TEST_BOOL(et.IsEmpty());
    et.Sample(plTime::Zero(), plTime::Seconds(1.0), result);

    PLASMA_TEST_BOOL(result.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Sample")
  {
    plEventTrack et;
    plHybridArray<plHashedString, 8> result;

    et.AddControlPoint(plTime::Seconds(3.0), "Event3");
    et.AddControlPoint(plTime::Seconds(0.0), "Event0");
    et.AddControlPoint(plTime::Seconds(4.0), "Event4");
    et.AddControlPoint(plTime::Seconds(1.0), "Event1");
    et.AddControlPoint(plTime::Seconds(2.0), "Event2");

    PLASMA_TEST_BOOL(!et.IsEmpty());

    // sampling an empty range should yield no results, even if sampling an exact time where an event is
    result.Clear();
    {{et.Sample(plTime::Seconds(0.0), plTime::Seconds(0.0), result);
    PLASMA_TEST_INT(result.GetCount(), 0);
  }

  {
    result.Clear();
    et.Sample(plTime::Seconds(1.0), plTime::Seconds(1.0), result);
    PLASMA_TEST_INT(result.GetCount(), 0);
  }

  {
    result.Clear();
    et.Sample(plTime::Seconds(4.0), plTime::Seconds(4.0), result);
    PLASMA_TEST_INT(result.GetCount(), 0);
  }
}

{
  result.Clear();
  et.Sample(plTime::Seconds(0.0), plTime::Seconds(1.0), result);
  PLASMA_TEST_INT(result.GetCount(), 1);
  PLASMA_TEST_STRING(result[0].GetString(), "Event0");
}

{
  result.Clear();
  et.Sample(plTime::Seconds(0.0), plTime::Seconds(2.0), result);
  PLASMA_TEST_INT(result.GetCount(), 2);
  PLASMA_TEST_STRING(result[0].GetString(), "Event0");
  PLASMA_TEST_STRING(result[1].GetString(), "Event1");
}

{
  result.Clear();
  et.Sample(plTime::Seconds(0.0), plTime::Seconds(4.0), result);
  PLASMA_TEST_INT(result.GetCount(), 4);
  PLASMA_TEST_STRING(result[0].GetString(), "Event0");
  PLASMA_TEST_STRING(result[1].GetString(), "Event1");
  PLASMA_TEST_STRING(result[2].GetString(), "Event2");
  PLASMA_TEST_STRING(result[3].GetString(), "Event3");
}

{
  result.Clear();
  et.Sample(plTime::Seconds(0.0), plTime::Seconds(10.0), result);
  PLASMA_TEST_INT(result.GetCount(), 5);
  PLASMA_TEST_STRING(result[0].GetString(), "Event0");
  PLASMA_TEST_STRING(result[1].GetString(), "Event1");
  PLASMA_TEST_STRING(result[2].GetString(), "Event2");
  PLASMA_TEST_STRING(result[3].GetString(), "Event3");
  PLASMA_TEST_STRING(result[4].GetString(), "Event4");
}

{
  result.Clear();
  et.Sample(plTime::Seconds(-0.1), plTime::Seconds(10.0), result);
  PLASMA_TEST_INT(result.GetCount(), 5);
  PLASMA_TEST_STRING(result[0].GetString(), "Event0");
  PLASMA_TEST_STRING(result[1].GetString(), "Event1");
  PLASMA_TEST_STRING(result[2].GetString(), "Event2");
  PLASMA_TEST_STRING(result[3].GetString(), "Event3");
  PLASMA_TEST_STRING(result[4].GetString(), "Event4");
}

et.Clear();
PLASMA_TEST_BOOL(et.IsEmpty());
}


PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Reverse Sample")
{
  plEventTrack et;
  plHybridArray<plHashedString, 8> result;

  et.AddControlPoint(plTime::Seconds(3.0), "Event3");
  et.AddControlPoint(plTime::Seconds(0.0), "Event0");
  et.AddControlPoint(plTime::Seconds(4.0), "Event4");
  et.AddControlPoint(plTime::Seconds(1.0), "Event1");
  et.AddControlPoint(plTime::Seconds(2.0), "Event2");

  {
    result.Clear();
    et.Sample(plTime::Seconds(2.0), plTime::Seconds(0.0), result);
    PLASMA_TEST_INT(result.GetCount(), 2);
    PLASMA_TEST_STRING(result[0].GetString(), "Event2");
    PLASMA_TEST_STRING(result[1].GetString(), "Event1");
  }

  {
    result.Clear();
    et.Sample(plTime::Seconds(4.0), plTime::Seconds(0.0), result);
    PLASMA_TEST_INT(result.GetCount(), 4);
    PLASMA_TEST_STRING(result[0].GetString(), "Event4");
    PLASMA_TEST_STRING(result[1].GetString(), "Event3");
    PLASMA_TEST_STRING(result[2].GetString(), "Event2");
    PLASMA_TEST_STRING(result[3].GetString(), "Event1");
  }

  {
    result.Clear();
    et.Sample(plTime::Seconds(10.0), plTime::Seconds(0.0), result);
    PLASMA_TEST_INT(result.GetCount(), 4);
    PLASMA_TEST_STRING(result[0].GetString(), "Event4");
    PLASMA_TEST_STRING(result[1].GetString(), "Event3");
    PLASMA_TEST_STRING(result[2].GetString(), "Event2");
    PLASMA_TEST_STRING(result[3].GetString(), "Event1");
  }

  {
    result.Clear();
    et.Sample(plTime::Seconds(10.0), plTime::Seconds(-0.1), result);
    PLASMA_TEST_INT(result.GetCount(), 5);
    PLASMA_TEST_STRING(result[0].GetString(), "Event4");
    PLASMA_TEST_STRING(result[1].GetString(), "Event3");
    PLASMA_TEST_STRING(result[2].GetString(), "Event2");
    PLASMA_TEST_STRING(result[3].GetString(), "Event1");
    PLASMA_TEST_STRING(result[4].GetString(), "Event0");
  }
}
}
