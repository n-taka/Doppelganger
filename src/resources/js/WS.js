import { WSTasks } from './WSTasks.js';

export const WS = {};

function onOpen(event) {
    console.log("connected. (websocket)");
}
function onMessage(event) {
    if (event && event.data) {
        const json = JSON.parse(event.data);
        console.log(json);

        if ("task" in json) {
            if (json["task"] in WSTasks) {
                WSTasks[json["task"]](json["parameters"]);
                // navigator.locks.request('websocketTask', async lock => {
                //     return wsCallbackFuncs[j["task"]](j);
                // });
            }
            else {
                console.log("task " + json["task"] + " is not yet implemented.");
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
    // window.open('about:blank','_self').close();
}

WS.init = async function () {
    // location.pathname.split('/')[1]: roomUUID
    const uri = (location.protocol == "http:" ? "ws://" : "wss://") + location.host + "/" + location.pathname.split('/')[1] + "/";
    if (WS.ws == null) {
        WS.ws = new WebSocket(uri);
        WS.ws.onopen = onOpen;
        WS.ws.onmessage = onMessage;
        WS.ws.onclose = onClose;
        WS.ws.onerror = onError;
    }

    // when we close tab/window, we gracefully close websocket
    // this is not reliable??
    window.addEventListener('beforeunload', function (e) {
        WS.ws.onclose = function () { };
        WS.ws.close();
    });
    return;
};

WS.sendMsg = async function (msg) {
    if (WS.ws.readyState == 1) {
        WS.ws.send(msg);
        return;
    }
    else {
        throw new Error("cannot sendMsg (WS)");
    }
}

