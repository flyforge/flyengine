/*SOURCE-HASH:8BE199C1BF84B856*/
"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var Consumable;
(function (Consumable) {
    Consumable[Consumable["Health"] = 0] = "Health";
    //Armor = 1,
    Consumable[Consumable["AmmoTypes_Start"] = 2] = "AmmoTypes_Start";
    Consumable[Consumable["Ammo_None"] = 3] = "Ammo_None";
    Consumable[Consumable["Ammo_Pistol"] = 4] = "Ammo_Pistol";
    Consumable[Consumable["Ammo_MachineGun"] = 5] = "Ammo_MachineGun";
    Consumable[Consumable["Ammo_Shotgun"] = 6] = "Ammo_Shotgun";
    Consumable[Consumable["Ammo_Plasma"] = 7] = "Ammo_Plasma";
    Consumable[Consumable["Ammo_Rocket"] = 8] = "Ammo_Rocket";
    Consumable[Consumable["AmmoTypes_End"] = 9] = "AmmoTypes_End";
})(Consumable = exports.Consumable || (exports.Consumable = {}));
var Weapon;
(function (Weapon) {
    Weapon[Weapon["None"] = 0] = "None";
    Weapon[Weapon["Pistol"] = 1] = "Pistol";
    Weapon[Weapon["Shotgun"] = 2] = "Shotgun";
    Weapon[Weapon["MachineGun"] = 3] = "MachineGun";
    Weapon[Weapon["PlasmaRifle"] = 4] = "PlasmaRifle";
    Weapon[Weapon["RocketLauncher"] = 5] = "RocketLauncher";
})(Weapon = exports.Weapon || (exports.Weapon = {}));
;
exports.MaxConsumableAmount = [];
exports.MaxConsumableAmount[Consumable.Health] = 100;
exports.MaxConsumableAmount[Consumable.Ammo_Pistol] = 50;
exports.MaxConsumableAmount[Consumable.Ammo_MachineGun] = 150;
exports.MaxConsumableAmount[Consumable.Ammo_Shotgun] = 40;
exports.MaxConsumableAmount[Consumable.Ammo_Plasma] = 100;
exports.MaxConsumableAmount[Consumable.Ammo_Rocket] = 20;
