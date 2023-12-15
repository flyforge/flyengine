import pl = require("TypeScript/pl")
import _gm = require("Scripting/GameMessages")

export class UnlockWeapon extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    WeaponType: number = 0;
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgTriggerTriggered, "OnMsgTriggerTriggered");
    }

    OnMsgTriggerTriggered(msg: pl.MsgTriggerTriggered): void {

        if (msg.TriggerState == pl.TriggerState.Activated && msg.Message == "Pickup") {

            // TODO: need GO handles in messages to identify who entered the trigger
            let player = pl.World.TryGetObjectWithGlobalKey("Player");
            if (player == null)
                return;

            let hm = new _gm.MsgUnlockWeapon();
            hm.WeaponType = this.WeaponType;

            player.SendMessage(hm, true);

            if (hm.return_consumed == false)
                return;

            let del = new pl.MsgDeleteGameObject();
            this.GetOwner().PostMessage(del, 0.1);
        }
    }
}

