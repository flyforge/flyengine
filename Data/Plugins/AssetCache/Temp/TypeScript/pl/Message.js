/*SOURCE-HASH:6F9A2CA3005CF5FF*/
"use strict";
var __extends = (this && this.__extends) || (function () {
    var extendStatics = function (d, b) {
        extendStatics = Object.setPrototypeOf ||
            ({ __proto__: [] } instanceof Array && function (d, b) { d.__proto__ = b; }) ||
            function (d, b) { for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p]; };
        return extendStatics(d, b);
    };
    return function (d, b) {
        extendStatics(d, b);
        function __() { this.constructor = d; }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
/**
 * Base class for all message types.
 */
var Message = /** @class */ (function () {
    function Message() {
        this.TypeNameHash = 0;
    }
    Message.GetTypeNameHash = function () { return 0; };
    return Message;
}());
exports.Message = Message;
/**
 * Base class for all message types that are broadcast as 'events',
 * ie. bubbling up the scene graph, instead of being delivered downwards the graph structure.
 */
var EventMessage = /** @class */ (function (_super) {
    __extends(EventMessage, _super);
    function EventMessage() {
        return _super !== null && _super.apply(this, arguments) || this;
    }
    return EventMessage;
}(Message));
exports.EventMessage = EventMessage;
