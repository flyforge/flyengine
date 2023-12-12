import pl = require("TypeScript/pl")
import PLASMA_TEST = require("./TestFramework")
import shared = require("./Shared")

export class HelperComponent extends pl.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgGenericEvent, "OnMsgGenericEvent");
    }

    // Initialize(): void { }
    // Deinitialize(): void { }
    // OnActivated(): void { }
    // OnDeactivated(): void { }

    OnSimulationStarted(): void {
        this.SetTickInterval(pl.Time.Milliseconds(0));
    }

    RaiseEvent(text: string): void {
        let e = new pl.MsgGenericEvent;
        e.Message = text;
        this.BroadcastEvent(e);
    }

    OnMsgGenericEvent(msg: pl.MsgGenericEvent): void {

        if (msg.Message == "Event1") {

            this.RaiseEvent("e1");
        }

        // should not reach itself
        PLASMA_TEST.BOOL(msg.Message != "e1");
    }

    Tick(): void {
    }
}

