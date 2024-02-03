import pl = require("TypeScript/pl")

export class JointMotorFlip extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    Seconds: number = 10;
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        // you can only call "RegisterMessageHandler" from within this function
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgComponentInternalTrigger, "OnMsgComponentInternalTrigger");
    }

    OnSimulationStarted(): void {

        let msg = new pl.MsgComponentInternalTrigger();
        msg.Message = "FlipMotor";

        this.PostMessage(msg, this.Seconds);

    }

    OnMsgComponentInternalTrigger(msg: pl.MsgComponentInternalTrigger): void {

        let joint = this.GetOwner().TryGetComponentOfBaseType(pl.JoltHingeConstraintComponent);

        if (joint != null) {

            joint.DriveTargetValue = -joint.DriveTargetValue;
        }

        this.PostMessage(msg, this.Seconds);
    }
}

