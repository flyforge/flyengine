import pl = require("../../TypeScript/pl")

export class HealthPickup extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    pfxTop: pl.ParticleComponent = null;
    pfxPickup: pl.ParticleComponent = null;

    OnSimulationStarted(): void {
        this.pfxTop = this.GetOwner().FindChildByName("Particle", true).TryGetComponentOfBaseType(pl.ParticleComponent);
        this.pfxPickup = this.GetOwner().TryGetComponentOfBaseType(pl.ParticleComponent);
    }

    static RegisterMessageHandlers() {

        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgGenericEvent, "OnMsgGenericEvent");
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgTriggerTriggered, "OnMsgTriggerTriggered");
    }

    OnMsgGenericEvent(msg: pl.MsgGenericEvent): void {

        if (msg.Message == "Animation Cue 1") {
            this.pfxTop.StartEffect();
        }
    }    

    OnMsgTriggerTriggered(msg: pl.MsgTriggerTriggered): void {

        if (msg.TriggerState == pl.TriggerState.Activated && msg.Message == "Pickup") {

            this.pfxPickup.StartEffect();

            let del = new pl.MsgDeleteGameObject();
            this.GetOwner().PostMessage(del, 0.1);
        }
    }    
}

