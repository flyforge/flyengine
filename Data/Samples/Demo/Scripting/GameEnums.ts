import pl = require("TypeScript/pl")

export enum Consumable {
    Health = 0,
    //Armor = 1,

    AmmoTypes_Start = 2,
    Ammo_None = 3,
    Ammo_Pistol = 4,
    Ammo_Shotgun = 6,
    AmmoTypes_End = 7,
}

export enum Weapon {
    None = 0,
    Pistol = 1,
    Shotgun = 2,
};

export let MaxConsumableAmount: number[] = []

MaxConsumableAmount[Consumable.Health] = 100;
MaxConsumableAmount[Consumable.Ammo_Pistol] = 50;
MaxConsumableAmount[Consumable.Ammo_Shotgun] = 40;

