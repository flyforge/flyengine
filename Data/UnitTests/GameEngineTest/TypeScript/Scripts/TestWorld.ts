import pl = require("TypeScript/pl")
import PLASMA_TEST = require("./TestFramework")

export class TestWorld extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgGenericEvent, "OnMsgGenericEvent");
    }

    foundObjs: pl.GameObject[] = [];

    FoundObjectCallback = (go: pl.GameObject): boolean => {

        this.foundObjs.push(go);
        return true;
    }

    ExecuteTests(): boolean {

        // FindObjectsInSphere
        {
            this.foundObjs = [];
            pl.World.FindObjectsInSphere("Category1", new pl.Vec3(5, 0, 0), 3, this.FoundObjectCallback);
            PLASMA_TEST.INT(this.foundObjs.length, 2);

            this.foundObjs = [];
            pl.World.FindObjectsInSphere("Category2", new pl.Vec3(5, 0, 0), 3, this.FoundObjectCallback);
            PLASMA_TEST.INT(this.foundObjs.length, 1);
        }

        // FindObjectsInBox
        {
            this.foundObjs = [];
            pl.World.FindObjectsInBox("Category1", new pl.Vec3(-10, 0, -5), new pl.Vec3(0, 10, 5), this.FoundObjectCallback);
            PLASMA_TEST.INT(this.foundObjs.length, 3);

            this.foundObjs = [];
            pl.World.FindObjectsInBox("Category2", new pl.Vec3(-10, 0, -5), new pl.Vec3(0, 10, 5), this.FoundObjectCallback);
            PLASMA_TEST.INT(this.foundObjs.length, 2);
        }

        return false;
    }

    OnMsgGenericEvent(msg: pl.MsgGenericEvent): void {

        if (msg.Message == "TestWorld") {

            if (this.ExecuteTests()) {
                msg.Message = "repeat";
            }
            else {
                msg.Message = "done";
            }
        
        }
    }

}

