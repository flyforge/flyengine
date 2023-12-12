#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/JSONReader.h>


plJSONReader::plJSONReader()
{
  m_bParsingError = false;
}

plResult plJSONReader::Parse(plStreamReader& ref_inputStream, plUInt32 uiFirstLineOffset)
{
  m_bParsingError = false;
  m_Stack.Clear();
  m_sLastName.Clear();

  SetInputStream(ref_inputStream, uiFirstLineOffset);

  while (!m_bParsingError && ContinueParsing())
  {
  }

  if (m_bParsingError)
  {
    m_Stack.Clear();
    m_Stack.PushBack(Element());

    return PLASMA_FAILURE;
  }

  // make sure there is one top level element
  if (m_Stack.IsEmpty())
    m_Stack.PushBack(Element());

  return PLASMA_SUCCESS;
}

bool plJSONReader::OnVariable(plStringView sVarName)
{
  m_sLastName = sVarName;

  return true;
}

void plJSONReader::OnReadValue(plStringView sValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementMode::Array)
    m_Stack.PeekBack().m_Array.PushBack(std::move(plString(sValue)));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = std::move(plString(sValue));

  m_sLastName.Clear();
}

void plJSONReader::OnReadValue(double fValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementMode::Array)
    m_Stack.PeekBack().m_Array.PushBack(plVariant(fValue));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = plVariant(fValue);

  m_sLastName.Clear();
}

void plJSONReader::OnReadValue(bool bValue)
{
  if (m_Stack.PeekBack().m_Mode == ElementMode::Array)
    m_Stack.PeekBack().m_Array.PushBack(plVariant(bValue));
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = plVariant(bValue);

  m_sLastName.Clear();
}

void plJSONReader::OnReadValueNULL()
{
  if (m_Stack.PeekBack().m_Mode == ElementMode::Array)
    m_Stack.PeekBack().m_Array.PushBack(plVariant());
  else
    m_Stack.PeekBack().m_Dictionary[m_sLastName] = plVariant();

  m_sLastName.Clear();
}

void plJSONReader::OnBeginObject()
{
  m_Stack.PushBack(Element());
  m_Stack.PeekBack().m_Mode = ElementMode::Dictionary;
  m_Stack.PeekBack().m_sName = m_sLastName;

  m_sLastName.Clear();
}

void plJSONReader::OnEndObject()
{
  Element& Child = m_Stack[m_Stack.GetCount() - 1];

  if (m_Stack.GetCount() > 1)
  {
    Element& Parent = m_Stack[m_Stack.GetCount() - 2];

    if (Parent.m_Mode == ElementMode::Array)
    {
      Parent.m_Array.PushBack(Child.m_Dictionary);
    }
    else
    {
      Parent.m_Dictionary[Child.m_sName] = std::move(Child.m_Dictionary);
    }

    m_Stack.PopBack();
  }
  else
  {
    // do nothing, keep the top-level dictionary
  }
}

void plJSONReader::OnBeginArray()
{
  m_Stack.PushBack(Element());
  m_Stack.PeekBack().m_Mode = ElementMode::Array;
  m_Stack.PeekBack().m_sName = m_sLastName;

  m_sLastName.Clear();
}

void plJSONReader::OnEndArray()
{
  Element& Child = m_Stack[m_Stack.GetCount() - 1];
  Element& Parent = m_Stack[m_Stack.GetCount() - 2];

  if (Parent.m_Mode == ElementMode::Array)
  {
    Parent.m_Array.PushBack(Child.m_Array);
  }
  else
  {
    Parent.m_Dictionary[Child.m_sName] = std::move(Child.m_Array);
  }

  m_Stack.PopBack();
}



void plJSONReader::OnParsingError(plStringView sMessage, bool bFatal, plUInt32 uiLine, plUInt32 uiColumn)
{
  m_bParsingError = true;
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_JSONReader);
