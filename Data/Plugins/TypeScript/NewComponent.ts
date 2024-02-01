import pl = require("<PATH-TO-pl-TS>")

//export class NewComponent extends pl.TypescriptComponent {
export class NewComponent extends pl.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        // you can only call "RegisterMessageHandler" from within this function
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgSetColor, "OnMsgSetColor");
    }

    // Initialize(): void { }
    // Deinitialize(): void { }
    // OnActivated(): void { }
    // OnDeactivated(): void { }

    OnSimulationStarted(): void {
        this.SetTickInterval(pl.Time.Milliseconds(100));
    }

    OnMsgSetColor(msg: pl.MsgSetColor): void {
        pl.Log.Info("MsgSetColor: " + msg.Color.r + ", " + msg.Color.g + ", " + msg.Color.b + ", " + msg.Color.a);
    }

    Tick(): void {
        // if a regular tick is not needed, remove this and derive directly from pl.TypescriptComponent
        pl.Log.Info("NewComponent.Tick()")
    }
}

