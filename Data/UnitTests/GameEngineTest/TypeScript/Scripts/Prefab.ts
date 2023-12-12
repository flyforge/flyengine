import pl = require("TypeScript/pl")

export class Prefab extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    NumberVar: number = 11;
    BoolVar: boolean = true;
    StringVar: string = "Hello";
    Vec3Var: pl.Vec3 = new pl.Vec3(1, 2, 3);
    ColorVar: pl.Color = new pl.Color(0.768151, 0.142913, 0.001891, 1);
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

    }
}

