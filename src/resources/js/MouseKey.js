import * as THREE from 'https://unpkg.com/three@0.126.0/build/three.module.js';
import { Core } from './Core.js';
import { WS } from './WS.js';

export const MouseKey = {};

MouseKey.init = function () {
    ////
    // icon
    MouseKey.iconIdx = Math.floor(Math.random() * 10);
    document.body.style.cursor = "url(../icon/cursorIcon" + MouseKey.iconIdx + ".png) 16 16 , default"

    ////
    // pointer event
    // canvas.controls.domElement.addEventListener?
    // disable right click
    document.addEventListener("contextmenu", function (e) { e.preventDefault(); });
    // update strokeTimeStamp on pointerDown
    document.addEventListener("pointerdown", function (e) {
        e.preventDefault();
        MouseKey.strokeTimeStamp = Date.now();
    });
    // we first updateCursor, then syncCursor
    document.addEventListener("pointermove", function (e) { e.preventDefault(); MouseKey.updateCursor(e) });
    document.addEventListener("pointermove", function (e) { e.preventDefault(); MouseKey.syncCursor(false) });
    // document.addEventListener("pointerup", customClick);
    MouseKey["cursors"] = {};
    MouseKey["prevCursor"] = new THREE.Vector2(-1.0, -1.0);

    ////
    // keyboard events
    document.addEventListener("keyup", (function (e) {
        DoppelCore.strokeTimeStamp = Date.now();
        var keycode = e.code;
        if (keycode == 'KeyF') {
            // 'f' key
            // fitToFrame();
        }
        else if (keycode == 'Digit0' || keycode == 'NumPad0') {
            // alignCamera(0 + ((e.altKey) ? 3 : 0));
        }
        else if (keycode == 'Digit1' || keycode == 'NumPad1') {
            // alignCamera(1 + ((e.altKey) ? 3 : 0));
        }
        else if (keycode == 'Digit2' || keycode == 'NumPad2') {
            // alignCamera(2 + ((e.altKey) ? 3 : 0));
        }
        else {
            // currently, other keyboard shortcut is not implemented.
        }
    }));

    // we explicitly useCapture to perform syncCursor BEFORE WS is closed.
    window.addEventListener('beforeunload', function () {
        syncCursors(true);
    }, true);
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

MouseKey.syncCursor = function (remove = false) {
    const cursorNEq = (Core["UUID"]
        && MouseKey["cursors"][Core["UUID"]]
        && !MouseKey["prevCursor"].equals(MouseKey["cursors"][Core["UUID"]]["dir"]));

    var json = {
        "task": "syncCursor",
        "UUID": Core["UUID"],
        "timestamp": MouseKey.strokeTimeStamp,
        "remove": remove
    };
    if (cursorNEq) {
        json["cursor"] = {
            "dirX": MouseKey["cursors"][Core["UUID"]]["dir"].x,
            "dirY": MouseKey["cursors"][Core["UUID"]]["dir"].y
        };
        MouseKey["prevCursor"] = MouseKey["cursors"][Core["UUID"]]["dir"].clone();
    }

    const msg = JSON.stringify(json);
    WS.sendMsg(msg);
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

