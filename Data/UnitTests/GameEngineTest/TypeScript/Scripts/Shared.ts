import pl = require("TypeScript/pl")

export class MyMessage extends pl.Message {
    PLASMA_DECLARE_MESSAGE_TYPE;

    text: string = "hello";
}

export class MyMessage2 extends pl.Message {
    PLASMA_DECLARE_MESSAGE_TYPE;

    value: number = 0;
}

