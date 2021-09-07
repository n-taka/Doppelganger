export var WS = {};

function onOpen(event) {
    console.log("connected. (websocket)");
}
function onMessage(event) {
    if (event && event.data) {
        const json = JSON.parse(event.data);
        console.log(json);

        if ("task" in json) {
            if (json["task"] in wsCallbackFuncs) {
                // do some APIcall
                // json["task"]
                // json["parameters"]

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

WS.init = function () {
    // location.pathname.split('/')[1]: roomUUID
    if (location.protocol == "http:") {
        var uri = "ws://" + location.host + "/" + location.pathname.split('/')[1] + "/"
    }
    else if (location.protocol == "https:") {
        var uri = "wss://" + location.host + "/" + location.pathname.split('/')[1] + "/"
    }
    if (WS.ws == null) {
        WS.ws = new WebSocket(uri);
        WS.ws.onopen = onOpen;
        WS.ws.onmessage = onMessage;
        WS.ws.onclose = onClose;
        WS.ws.onerror = onError;
    }
};

WS.sendMsg = function (msg) {
    if (WS.ws.readyState == 1) {
        WS.ws.send(msg);
    }
}

