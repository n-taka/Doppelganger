import * as THREE from 'https://unpkg.com/three@0.126.0/build/three.module.js';
import { DoppelCore } from './DoppelCore.js';
import { canvas, fitToFrame, alignCamera, resetCamera } from './canvas.js';
import { DoppelWS } from './websocket.js';

export var mouseCursors = {};

export var mouseKey = {};

mouseKey.init = function () {
    // listeners related with pointerinteraction
    // custom cursor
    // should be document.addEventListener??
    canvas.controls.domElement.addEventListener("pointermove", customCursor, false);
    // right click
    canvas.controls.domElement.addEventListener("contextmenu", function (e) { e.preventDefault(); }, false);
    canvas.controls.domElement.addEventListener("pointerup", customClick, false);
    // websocket messaging
    canvas.controls.domElement.addEventListener("pointerdown", function (e) {
        e.preventDefault(); DoppelCore.strokeTimeStamp = Date.now();
    }, false);
    // D&D upload
    canvas.controls.domElement.addEventListener("dragover", function (e) { e.preventDefault(); }, false);
    // keyboard shortcuts
    document.addEventListener("keyup", (function (e) {
        DoppelCore.strokeTimeStamp = Date.now();
        var keycode = e.code;
        if (keycode == 'KeyF') {
            // 'f' key
            fitToFrame();
        }
        else if (keycode == 'Digit0' || keycode == 'NumPad0') {
            alignCamera(0 + ((e.altKey) ? 3 : 0));
        }
        else if (keycode == 'Digit1' || keycode == 'NumPad1') {
            alignCamera(1 + ((e.altKey) ? 3 : 0));
        }
        else if (keycode == 'Digit2' || keycode == 'NumPad2') {
            alignCamera(2 + ((e.altKey) ? 3 : 0));
        }
        else {
            // currently, other keyboard shortcut is not implemented.
        }
    }));

    window.onbeforeunload = function () {
        syncCursors(true);
    };
};

function customCursor(e) {
    e.preventDefault();
    if (DoppelCore.sessionId > -1 && mouseCursors[DoppelCore.sessionId] == null) {
        mouseCursors[DoppelCore.sessionId] = { "dir": new THREE.Vector2(0, 0), "img": new Image() };
        var style = mouseCursors[DoppelCore.sessionId].img.style;
        style.position = "fixed";
        style["z-index"] = "1000"; // material css sidenav has 999
        style["pointer-events"] = "none";
        mouseCursors[DoppelCore.sessionId].img.src = "../icon/cursorIcon" + (DoppelCore.sessionId % 10) + ".png";

        // for cursor of itself, we use css based approach (for better UX!)
        // document.body.appendChild(mouseCursors[sessionId].img);
    }
    if (mouseCursors[DoppelCore.sessionId]) {
        mouseCursors[DoppelCore.sessionId].img.style.left = (e.clientX - 16) + "px";
        mouseCursors[DoppelCore.sessionId].img.style.top = (e.clientY - 16) + "px";

        var mouse = new THREE.Vector2();
        var clientX = e.clientX;
        var clientY = e.clientY;
        mouse.x = clientX - window.innerWidth / 2.0;
        mouse.y = clientY - window.innerHeight / 2.0;

        mouseCursors[DoppelCore.sessionId]["dir"] = mouse.clone();

        syncCursors();
    }
    resetCamera(false);
}

function customClick(e) {
    e.preventDefault();
    if (e.button > 0) {
        var mouse = new THREE.Vector2();
        var clientX = e.clientX;
        var clientY = e.clientY;
        mouse.x = clientX - window.innerWidth / 2.0;
        mouse.y = clientY - window.innerHeight / 2.0;

        if (e.button == 0) {
            // left click
            // left click cannot used. somehow...
        }
        else if (e.button == 2) {
            // right click
        }
        else if (e.button == 1) {
            // middle click
        }
    }
}

function syncCursors(remove = false) {
    var cursorEq = (canvas.lastCursorDir != null
        && mouseCursors[DoppelCore.sessionId] != null
        && !canvas.lastCursorDir.equals(mouseCursors[DoppelCore.sessionId]["dir"]));

    var jsonObj = {
        "sessionId": DoppelCore.sessionId,
        "timestamp": -1.0,
        "task": "syncCursors",
        "remove": remove
    };
    if (cursorEq) {
        jsonObj["cursor"] = {
            "dirX": mouseCursors[DoppelCore.sessionId]["dir"].x,
            "dirY": mouseCursors[DoppelCore.sessionId]["dir"].y
        };
        canvas.lastCursorDir = mouseCursors[DoppelCore.sessionId]["dir"].clone();
    }

    var msg = JSON.stringify(jsonObj);
    DoppelWS.sendMsg(msg);
}

