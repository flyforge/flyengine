/*SOURCE-HASH:E6DA48458F3A5E94*/
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
var __GameObject = require("./GameObject");
exports.GameObject = __GameObject.GameObject;
var __Message = require("./Message");
exports.Message = __Message.Message;
exports.EventMessage = __Message.EventMessage;
var __Time = require("./Time");
exports.Time = __Time.Time;
/**
 * Abstract base class for all component classes.
 *
 * TypeScript instances to components act as weak pointers. Components are always created, managed and destroyed on the C++ side.
 * The TypeScript side can only ask the C++ side to create, destroy or modify components. If a component is destroyed on the
 * C++ side, the TS side is not informed. However, calling 'IsValid()' allows one to query whether the component is still alive
 * on the C++ side. The IsValid() result stays true at least until the end of the frame.
 *
 * Executing operations other than 'IsValid()' on an invalid component is an error.
 */
var Component = /** @class */ (function () {
    function Component() {
    }
    /**
     * Checks whether a component is still alive on the C++ side.
     *
     * @returns True for live components. False for components that have been destroyed already on the C++ side.
     */
    Component.prototype.IsValid = function () {
        return __CPP_Component_IsValid(this);
    };
    /**
     * Returns the owning game object of this component.
     */
    Component.prototype.GetOwner = function () {
        return __CPP_Component_GetOwner(this);
    };
    /**
     * Sets the active flag for a component, which affects the component's final 'active state'.
     * Inactive components are present, but do not have any effect.
     */
    Component.prototype.SetActiveFlag = function (enabled) {
        __CPP_Component_SetActiveFlag(this, enabled);
    };
    /**
     * Checks whether the active flag is set on this component. See 'SetActiveFlag()' and 'IsActive()'.
     */
    Component.prototype.GetActiveFlag = function () {
        return __CPP_Component_GetActiveFlag(this);
    };
    /**
     * Checks whether this component is in an 'active state'.
     * A component is in the 'active state' if it has the active flag and its owning object is also in the active state.
     *
     *  See 'SetActiveFlag()'.
     */
    Component.prototype.IsActive = function () {
        return __CPP_Component_IsActive(this);
    };
    /**
     * Checks whether this component is both active and has already been initialized.
     */
    Component.prototype.IsActiveAndInitialized = function () {
        return __CPP_Component_IsActiveAndInitialized(this);
    };
    /**
     * Checks whether this component is active and has already been configured for game simulation.
     */
    Component.prototype.IsActiveAndSimulating = function () {
        return __CPP_Component_IsActiveAndSimulating(this);
    };
    /**
     * Sends an pl.Message directly to this component.
     *
     * @param expectResultData If set to true, the calling code assumes that the message receiver(s) may write result data
     *   back into the message and thus the caller is interested in reading that data afterwards. If set to false
     *   (the default) the state of the message is not synchronized back into the TypeScript message after the message
     *   has been delivered and thus any data written into the message by the receiver, is lost.
     */
    Component.prototype.SendMessage = function (msg, expectResultData) {
        if (expectResultData === void 0) { expectResultData = false; }
        __CPP_Component_SendMessage(this, msg.TypeNameHash, msg, expectResultData);
    };
    /**
     * Sends and pl.Message to this component.
     * The message is queued and delivered during the next convenient game update phase.
     * It may optionally be sent with a time delay.
     */
    Component.prototype.PostMessage = function (msg, delay) {
        if (delay === void 0) { delay = exports.Time.Zero(); }
        __CPP_Component_PostMessage(this, msg.TypeNameHash, msg, delay);
    };
    /**
     * Returns an ID unique to this component.
     */
    Component.prototype.GetUniqueID = function () {
        return __CPP_Component_GetUniqueID(this);
    };
    /*
        These functions are pretty much virtual for derived classes to override.
        However, to prevent unnecessary performance overhead, they are not implemented as virtual here.
        Instead just implement them in derived classes as needed.

        Initialize(): void {
        }
    
        Deinitialize(): void {
        }
    
        OnActivated(): void {
        }
    
        OnDeactivated(): void {
        }
    
        OnSimulationStarted(): void {
        }
    */
    Component.GetTypeNameHash = function () { return 0; };
    return Component;
}());
exports.Component = Component;
/**
 * Base class for all component types implemented exclusively with TypeScript.
 */
var TypescriptComponent = /** @class */ (function (_super) {
    __extends(TypescriptComponent, _super);
    function TypescriptComponent() {
        return _super !== null && _super.apply(this, arguments) || this;
    }
    // static RegisterMessageHandlers() {
    // }
    /**
     * Registers a member function to handle certain message types.
     * This must be called exclusively from a static function called 'RegisterMessageHandlers()'
     * to hook up a member function as a message handler to tell the C++ side which messages to
     * deliver to th TS side for message handling.
     *
     * @param msgType The TS type of the message to handle. E.g. 'pl.MsgSetColor'
     * @param handlerFuncName The function to call when messages of the given type arrive.
     */
    TypescriptComponent.RegisterMessageHandler = function (msgType, handlerFuncName) {
        __CPP_Binding_RegisterMessageHandler(msgType.GetTypeNameHash(), handlerFuncName);
    };
    /**
     * Broadcasts and event message up the graph (ie. to parent nodes) and to the next event message handler.
     */
    TypescriptComponent.prototype.BroadcastEvent = function (msg) {
        __CPP_TsComponent_BroadcastEvent(this, msg.TypeNameHash, msg);
    };
    return TypescriptComponent;
}(Component));
exports.TypescriptComponent = TypescriptComponent;
/**
 * Base class for all component types implemented exclusively with TypeScript, that need a regular update call.
 */
var TickedTypescriptComponent = /** @class */ (function (_super) {
    __extends(TickedTypescriptComponent, _super);
    function TickedTypescriptComponent() {
        return _super !== null && _super.apply(this, arguments) || this;
    }
    /**
     * Sets the time that passes between calls to 'Tick()'.
     * If set to zero, Tick() is called every frame, independent of frame-rate.
     *
     * @param seconds Time that shall pass between calls to 'Tick()'. Default is one second.
     */
    TickedTypescriptComponent.prototype.SetTickInterval = function (seconds) {
        __CPP_TsComponent_SetTickInterval(this, seconds);
    };
    return TickedTypescriptComponent;
}(TypescriptComponent));
exports.TickedTypescriptComponent = TickedTypescriptComponent;
