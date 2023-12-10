import pl = require("TypeScript/pl")

export class JointBroke extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    OnBreakMsg: string = "Joint Broke !";
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgPhysicsJointBroke, "OnMsgPhysicsJointBroke");
    }

    OnMsgPhysicsJointBroke(msg: pl.MsgPhysicsJointBroke): void {
        pl.Log.Info(this.OnBreakMsg);
    }
}

