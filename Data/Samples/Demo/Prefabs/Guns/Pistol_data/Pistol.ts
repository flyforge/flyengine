import pl = require("TypeScript/pl")
import _ge = require("Scripting/GameEnums")
import guns = require("Prefabs/Guns/Gun")

export class Pistol extends guns.Gun {


    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()

        this.singleShotPerTrigger = true;
    }

    static RegisterMessageHandlers() {

        guns.Gun.RegisterMessageHandlers();

        //pl.TypescriptComponent.RegisterMessageHandler(pl.MsgSetColor, "OnMsgSetColor");
    }

    nextAmmoPlus1Time: number = 0;

    OnSimulationStarted(): void {

        super.OnSimulationStarted();

        this.SetTickInterval(0);
    }

    Tick(): void {

        const now = pl.Time.GetGameTime();

        if (this.nextAmmoPlus1Time < now) {
            this.ammoInClip = pl.Utils.Clamp(this.ammoInClip + 1, 0, this.GetAmmoClipSize());

            this.nextAmmoPlus1Time = now + pl.Time.Seconds(0.2);
        }
    }

    GetAmmoType(): _ge.Consumable {
        return _ge.Consumable.Ammo_None;
    }

    GetAmmoClipSize(): number {
        return 8;
    }

    Fire(): void {

        let spawn = this.GetOwner().FindChildByName("Spawn").TryGetComponentOfBaseType(pl.SpawnComponent);
        if (spawn.CanTriggerManualSpawn() == false)
            return;

        this.nextAmmoPlus1Time = pl.Time.GetGameTime() + pl.Time.Seconds(0.5);

        this.ammoInClip -= 1;

        spawn.TriggerManualSpawn(false, pl.Vec3.ZeroVector());

        this.PlayShootSound();

    }

    RenderCrosshair(): void {
        // render nothing, have a laser pointer already
    }
}

