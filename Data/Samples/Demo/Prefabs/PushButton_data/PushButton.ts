import pl = require("TypeScript/pl")

export class PushButton extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    ButtonName: string = "";
    /* END AUTO-GENERATED: VARIABLES */

    slider: pl.TransformComponent = null;

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgGenericEvent, "OnMsgGenericEvent");
    }

    OnSimulationStarted(): void {

        let owner = this.GetOwner();
        let button = owner.FindChildByName("Button");
        this.slider = button.TryGetComponentOfBaseType(pl.TransformComponent);
    }

    OnMsgGenericEvent(msg: pl.MsgGenericEvent): void {

        if (msg.Message == "Use") {

            if (this.slider == null || this.slider.Running)
                return;

            this.slider.SetDirectionForwards(true);
            this.slider.Running = true;

            let newMsg = new pl.MsgGenericEvent();
            newMsg.Message = this.ButtonName;

            this.BroadcastEvent(newMsg);
        }
    }
}

