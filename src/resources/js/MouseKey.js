import * as THREE from 'https://cdn.skypack.dev/three';
import { Core } from './Core.js';
import { WS } from './WS.js';

export const MouseKey = {};

MouseKey.init = async function () {
    ////
    // icon
    MouseKey.iconIdx = Math.floor(Math.random() * 10);
    document.body.style.cursor = "url(../icon/cursorIcon" + MouseKey.iconIdx + ".png) 16 16 , default"

    // initialize timestamp
    MouseKey.strokeTimeStamp = Date.now();

    ////
    // pointer event
    // canvas.controls.domElement.addEventListener?
    // disable right click
    document.addEventListener("contextmenu", function (e) { e.preventDefault(); });
    // we first updateCursor, then syncCursor
    document.addEventListener("pointermove", function (e) { e.preventDefault(); MouseKey.updateCursor(e) });
    document.addEventListener("pointermove", function (e) { e.preventDefault(); MouseKey.syncCursor() });
    // document.addEventListener("pointerup", customClick);
    MouseKey["cursors"] = {};
    MouseKey["prevCursor"] = new THREE.Vector2(-1.0, -1.0);

    return;
};

MouseKey.updateCursor = function (e) {
    // if (DoppelCore.sessionId > -1 && mouseCursors[DoppelCore.sessionId] == null) {
    //     mouseCursors[DoppelCore.sessionId] = { "dir": new THREE.Vector2(0, 0), "img": new Image() };
    //     var style = mouseCursors[DoppelCore.sessionId].img.style;
    //     style.position = "fixed";
    //     style["z-index"] = "1000"; // material css sidenav has 999
    //     style["pointer-events"] = "none";
    //     mouseCursors[DoppelCore.sessionId].img.src = "../icon/cursorIcon" + (DoppelCore.sessionId % 10) + ".png";

    //     // for cursor of itself, we use css based approach (for better UX!)
    //     // document.body.appendChild(mouseCursors[sessionId].img);
    // }

    // mouseCursors[DoppelCore.sessionId].img.style.left = (e.clientX - 16) + "px";
    // mouseCursors[DoppelCore.sessionId].img.style.top = (e.clientY - 16) + "px";

    if (Core["UUID"]) {
        const mouse = new THREE.Vector2();
        mouse.x = e.clientX - window.innerWidth / 2.0;
        mouse.y = e.clientY - window.innerHeight / 2.0;
        const cursorInfo = {
            "idx": MouseKey.iconIdx,
            "dir": mouse
        };
        MouseKey["cursors"][Core["UUID"]] = cursorInfo;
    }
}

MouseKey.syncCursor = function () {
    const cursorNEq = (Core["UUID"]
        && MouseKey["cursors"][Core["UUID"]]
        && !MouseKey["prevCursor"].equals(MouseKey["cursors"][Core["UUID"]]["dir"]));

    const json = {
        "sessionUUID": Core["UUID"]
    };
    if (cursorNEq) {
        json["cursor"] = {
            "dir": {
                "x": MouseKey["cursors"][Core["UUID"]]["dir"].x,
                "y": MouseKey["cursors"][Core["UUID"]]["dir"].y
            },
            "idx": MouseKey["cursors"][Core["UUID"]]["idx"]
        };
        MouseKey["prevCursor"] = MouseKey["cursors"][Core["UUID"]]["dir"].clone();
        WS.sendMsg("syncCursor", json);
    }
}

// function customClick(e) {
//     e.preventDefault();
//     if (e.button > 0) {
//         var mouse = new THREE.Vector2();
//         var clientX = e.clientX;
//         var clientY = e.clientY;
//         mouse.x = clientX - window.innerWidth / 2.0;
//         mouse.y = clientY - window.innerHeight / 2.0;

//         if (e.button == 0) {
//             // left click
//             // left click cannot used. somehow...
//         }
//         else if (e.button == 2) {
//             // right click
//         }
//         else if (e.button == 1) {
//             // middle click
//         }
//     }
// }


