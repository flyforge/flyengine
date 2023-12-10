#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/Console/Console.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>

static void TelemetryMessage(void* pPassThrough)
{
  plTelemetryMessage Msg;
  plStringBuilder input;

  while (plTelemetry::RetrieveMessage('CMD', Msg) == PLASMA_SUCCESS)
  {
    if (Msg.GetMessageID() == 'EXEC' || Msg.GetMessageID() == 'COMP')
    {
      Msg.GetReader() >> input;

      if (plConsole::GetMainConsole())
      {
        if (auto pInt = plConsole::GetMainConsole()->GetCommandInterpreter())
        {
          plCommandInterpreterState s;
          s.m_sInput = input;

          plStringBuilder encoded;

          if (Msg.GetMessageID() == 'EXEC')
          {
            pInt->Interpret(s);
          }
          else
          {
            pInt->AutoComplete(s);
            encoded.AppendFormat(";;00||<{}", s.m_sInput);
          }

          for (const auto& l : s.m_sOutput)
          {
            encoded.AppendFormat(";;{}||{}", plArgI((plInt32)l.m_Type, 2, true), l.m_sText);
          }

          plTelemetryMessage msg;
          msg.SetMessageID('CMD', 'RES');
          msg.GetWriter() << encoded;
          plTelemetry::Broadcast(plTelemetry::Reliable, msg);
        }
      }
    }
  }
}

void AddConsoleEventHandler()
{
  plTelemetry::AcceptMessagesForSystem('CMD', true, TelemetryMessage, nullptr);
}

void RemoveConsoleEventHandler()
{
  plTelemetry::AcceptMessagesForSystem('CMD', false);
}
