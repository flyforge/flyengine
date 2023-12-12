import pl = require("TypeScript/pl")
import PLASMA_TEST = require("./TestFramework")

export class TestDebug extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgGenericEvent, "OnMsgGenericEvent");
    }

    ExecuteTests(): void {

        
        pl.Debug.RegisterCVar_Boolean("test.bool", true, "bool");
        PLASMA_TEST.BOOL(pl.Debug.ReadCVar_Boolean("test.bool") == true);
        pl.Debug.WriteCVar_Boolean("test.bool", false);
        PLASMA_TEST.BOOL(pl.Debug.ReadCVar_Boolean("test.bool") == false);
        
        pl.Debug.RegisterCVar_Int("test.int", 12, "int");
        PLASMA_TEST.BOOL(pl.Debug.ReadCVar_Int("test.int") == 12);
        pl.Debug.WriteCVar_Int("test.int", -12);
        PLASMA_TEST.BOOL(pl.Debug.ReadCVar_Int("test.int") == -12);
        
        pl.Debug.RegisterCVar_Float("test.float", 19, "float");
        PLASMA_TEST.BOOL(pl.Debug.ReadCVar_Float("test.float") == 19);
        pl.Debug.WriteCVar_Float("test.float", -19);
        PLASMA_TEST.BOOL(pl.Debug.ReadCVar_Float("test.float") == -19);

        pl.Debug.RegisterCVar_String("test.string", "hello", "string");
        PLASMA_TEST.BOOL(pl.Debug.ReadCVar_String("test.string") == "hello");
        pl.Debug.WriteCVar_String("test.string", "world");
        PLASMA_TEST.BOOL(pl.Debug.ReadCVar_String("test.string") == "world");
    }

    OnMsgGenericEvent(msg: pl.MsgGenericEvent): void {

        if (msg.Message == "TestDebug") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

