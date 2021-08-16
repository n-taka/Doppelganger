import { wsCallbackFuncs } from './websocketCallBackFuncs.js';


export var DoppelWS = {};

////
// local functions
////
function onOpen(event) {
    console.log("connected. (websocket)");
}
function onMessage(event) {
    if (event && event.data) {
        var j = JSON.parse(event.data);

        if ("task" in j) {
            if (j["task"] in wsCallbackFuncs) {
                navigator.locks.request('websocketTask', async lock => {
                    return wsCallbackFuncs[j["task"]](j);
                });
            }
            else {
                console.log("task " + j["task"] + " is not yet implemented.");
            }
        }
        else {
            console.log("json on websocket must have \"task\" entry!");
        }
    }
}

function onError(event) {
    console.log("Error... (websocket)");
}

function onClose(event) {
    console.log("disconnected... (websocket)");
    window.open('about:blank','_self').close();
}
////
// local functions (end)
////

DoppelWS.init = function () {
    if (location.protocol == "http:") {
        var uri = "ws://" + location.hostname + ":" + String(parseInt(location.port, 10)) + "/"
    }
    else if (location.protocol == "https:") {
        var uri = "wss://" + location.hostname + ":" + String(parseInt(location.port, 10)) + "/"
    }
    if (DoppelWS.ws == null) {
        DoppelWS.ws = new WebSocket(uri);
        DoppelWS.ws.onopen = onOpen;
        DoppelWS.ws.onmessage = onMessage;
        DoppelWS.ws.onclose = onClose;
        DoppelWS.ws.onerror = onError;
    }
};

DoppelWS.sendMsg = function (msg) {
    if (DoppelWS.ws.readyState == 1) {
        DoppelWS.ws.send(msg);
    }
}

