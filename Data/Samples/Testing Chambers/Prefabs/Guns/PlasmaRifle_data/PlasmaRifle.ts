import pl = require("TypeScript/pl")
import _ge = require("Scripting/GameEnums")
import gun = require("Prefabs/Guns/Gun")

export class PlasmaRifle extends gun.Gun {
    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()

        this.singleShotPerTrigger = false;
    }

    static RegisterMessageHandlers() {

        gun.Gun.RegisterMessageHandlers();

        //pl.TypescriptComponent.RegisterMessageHandler(pl.MsgSetColor, "OnMsgSetColor");
    }

    Tick(): void { }

    GetAmmoClipSize(): number {
        return 30;
    }

    GetAmmoType(): _ge.Consumable {
        return _ge.Consumable.Ammo_Plasma;
    }

    Fire(): void {

        let spawn = this.GetOwner().FindChildByName("Spawn").TryGetComponentOfBaseType(pl.SpawnComponent);
        if (spawn.CanTriggerManualSpawn() == false)
            return;

        this.ammoInClip -= 1;

        spawn.TriggerManualSpawn(true, pl.Vec3.ZeroVector());

        this.PlayShootSound();
    }

}

