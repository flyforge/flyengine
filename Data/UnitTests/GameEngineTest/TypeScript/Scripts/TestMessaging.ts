import pl = require("TypeScript/pl")
import PLASMA_TEST = require("./TestFramework")
import shared = require("./Shared")
import helper = require("./HelperComponent")

export class TestMessaging extends pl.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        pl.TypescriptComponent.RegisterMessageHandler(pl.MsgGenericEvent, "OnMsgGenericEvent");
        pl.TypescriptComponent.RegisterMessageHandler(shared.MyMessage, "OnMyMessage");
        pl.TypescriptComponent.RegisterMessageHandler(shared.MyMessage2, "OnMyMessage2");
    }

    step: number = 0;
    msgCount: number = 0;
    gotEvent: boolean = false;

    ExecuteTests(): boolean {

        if (this.step == 0) {
            let m = new shared.MyMessage();

            m.text = "hello 1";
            this.SendMessage(m, true);
            PLASMA_TEST.BOOL(m.text == "Got: hello 1");

            m.text = "hello 2";
            this.SendMessage(m, false);
            PLASMA_TEST.BOOL(m.text == "Got: hello 2");

            m.text = "hello 3";
            this.GetOwner().SendMessage(m, true);
            PLASMA_TEST.BOOL(m.text == "Got: hello 3");

            m.text = "hello 4";
            this.GetOwner().GetParent().SendMessageRecursive(m, true);
            PLASMA_TEST.BOOL(m.text == "Got: hello 4");

            return true;
        }

        if (this.step == 1) {
            let m = new shared.MyMessage2;
            m.value = 1;

            this.PostMessage(m);

            PLASMA_TEST.INT(this.msgCount, 0);

            return true;
        }

        if (this.step == 2) {

            PLASMA_TEST.INT(this.msgCount, 1);

            let m = new shared.MyMessage2;
            m.value = 1;

            this.GetOwner().PostMessage(m);

            PLASMA_TEST.INT(this.msgCount, 1);

            return true;
        }

        if (this.step == 3) {

            PLASMA_TEST.INT(this.msgCount, 2);

            let m = new shared.MyMessage2;
            m.value = 1;

            this.GetOwner().GetParent().PostMessage(m);

            PLASMA_TEST.INT(this.msgCount, 2);

            return true;
        }

        if (this.step == 4) {

            PLASMA_TEST.INT(this.msgCount, 2);

            let m = new shared.MyMessage2;
            m.value = 1;

            this.GetOwner().GetParent().PostMessageRecursive(m);

            PLASMA_TEST.INT(this.msgCount, 2);

            return true;
        }

        if (this.step == 5) {

            PLASMA_TEST.INT(this.msgCount, 3);

            let children = this.GetOwner().GetChildren();
            PLASMA_TEST.INT(children.length, 1);

            let hc: helper.HelperComponent = children[0].TryGetScriptComponent("HelperComponent");
            PLASMA_TEST.BOOL(hc != null);

            PLASMA_TEST.BOOL(!this.gotEvent);

            let te = new pl.MsgGenericEvent;
            te.Message = "Event1";

            hc.SendMessage(te);

            PLASMA_TEST.BOOL(this.gotEvent);
            this.gotEvent = false;

            hc.RaiseEvent("e1");
            PLASMA_TEST.BOOL(this.gotEvent);

            return true;
        }

        if (this.step == 6) {

            PLASMA_TEST.INT(this.msgCount, 3);

            return true;
        }

        return false;
    }

    OnMyMessage(msg: shared.MyMessage) {

        msg.text = "Got: " + msg.text;
    }

    OnMyMessage2(msg: shared.MyMessage2) {
        this.msgCount += msg.value;
    }

    OnMsgGenericEvent(msg: pl.MsgGenericEvent): void {

        if (msg.Message == "TestMessaging") {


            if (this.ExecuteTests()) {
                msg.Message = "repeat";
            }
            else {

                PLASMA_TEST.INT(this.msgCount, 3);

                msg.Message = "done";
            }

            this.step += 1;
        }

        if (msg.Message == "e1") {
            this.gotEvent = true;
        }
    }
}

