import pl = require("TypeScript/pl")

export class PhysicsLevelScript extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        // you can only call "RegisterMessageHandler" from within this function
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgTriggerTriggered, "OnMsgTriggerTriggered");
    }

    OnSimulationStarted(): void {
    }

    OnMsgTriggerTriggered(msg: pl.MsgTriggerTriggered): void {

        if (msg.Message == "ActivatePaddleWheel") {

            if (msg.TriggerState == pl.TriggerState.Activated) {

                let spawn1 = pl.World.TryGetObjectWithGlobalKey("PaddleWheelSpawn1");
                if (spawn1 != null) {
                    spawn1.SetActiveFlag(true);
                }

            }
            else if (msg.TriggerState == pl.TriggerState.Deactivated) {

                let spawn1 = pl.World.TryGetObjectWithGlobalKey("PaddleWheelSpawn1");
                if (spawn1 != null) {
                    spawn1.SetActiveFlag(false);
                }

            }
        }

        if (msg.Message == "ActivateSwing") {

            if (msg.TriggerState == pl.TriggerState.Activated) {

                let spawn1 = pl.World.TryGetObjectWithGlobalKey("SwingSpawn1");
                if (spawn1 != null) {
                    spawn1.SetActiveFlag(true);
                }

            }
            else if (msg.TriggerState == pl.TriggerState.Deactivated) {

                let spawn1 = pl.World.TryGetObjectWithGlobalKey("SwingSpawn1");
                if (spawn1 != null) {
                    spawn1.SetActiveFlag(false);
                }

            }
        }
    }
}

