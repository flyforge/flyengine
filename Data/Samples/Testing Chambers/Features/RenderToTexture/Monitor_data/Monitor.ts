import pl = require("TypeScript/pl")

export class MsgSwitchMonitor extends pl.Message {

    PL_DECLARE_MESSAGE_TYPE;

    renderTarget: string;
    screenMaterial: string;
}

export class Monitor extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        pl.TypescriptComponent.RegisterMessageHandler(MsgSwitchMonitor, "OnMsgSwitchMonitor");
    }

    OnMsgSwitchMonitor(msg: MsgSwitchMonitor): void {

        let owner = this.GetOwner();
        let display = owner.FindChildByName("Display");
        
        let mat = new pl.MsgSetMeshMaterial();
        mat.MaterialSlot = 0;
        mat.Material = msg.screenMaterial;

        display.SendMessage(mat);

        let activator = display.TryGetComponentOfBaseType(pl.RenderTargetActivatorComponent);
        activator.RenderTarget = msg.renderTarget;
    }
}

