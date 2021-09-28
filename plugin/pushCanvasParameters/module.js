import { Canvas } from '../../js/Canvas.js';
import { MouseKey } from '../../js/MouseKey.js';
import { WSTasks } from '../../js/WSTasks.js';

const pushCanvasParameters = function (parameters) {
    ////
    // [IN]
    // parameters = {
    //  "controls": {
    //   "target": {
    //    "x": x-coordinate,
    //    "y": y-coordinate,
    //    "z": z-coordinate,
    //    "timestamp": timestamp
    //   },
    //  },
    //  "camera": {
    //   "position": {
    //    "x": x-coordinate,
    //    "y": y-coordinate,
    //    "z": z-coordinate,
    //    "timestamp": timestamp
    //   },
    //   "up": {
    //    "x": x-coordinate,
    //    "y": y-coordinate,
    //    "z": z-coordinate,
    //    "timestamp": timestamp
    //   },
    //   "zoom": {
    //    "value": zoom parameter,
    //    "timestamp": timestamp
    //   }
    //  }
    // }

    let needUpdate = false;
    if ("controls" in parameters) {
        if ("target" in parameters["controls"]) {
            if (Canvas.lastControlTarget["timestamp"] <= parameters["controls"]["target"]["timestamp"]) {
                Canvas.controls.target.set(
                    parameters["controls"]["target"]["value"].x,
                    parameters["controls"]["target"]["value"].y,
                    parameters["controls"]["target"]["value"].z);
                Canvas.lastControlTarget["value"] = Canvas.controls.target.clone();
                Canvas.lastControlTarget["timestamp"] = parameters["controls"]["target"]["timestamp"];
                needUpdate = true;
            }
        }
    }
    if ("camera" in parameters) {
        if ("position" in parameters["camera"]) {
            if (Canvas.lastCameraPosition["timestamp"] <= parameters["camera"]["position"]["timestamp"]) {
                Canvas.camera.position.set(
                    parameters["camera"]["position"]["value"].x,
                    parameters["camera"]["position"]["value"].y,
                    parameters["camera"]["position"]["value"].z);
                Canvas.lastCameraPosition["value"] = Canvas.camera.position.clone();
                Canvas.lastCameraPosition["timestamp"] = parameters["camera"]["position"]["timestamp"];
                needUpdate = true;
            }
        }
        if ("up" in parameters["camera"]) {
            if (Canvas.lastCameraUp["timestamp"] <= parameters["camera"]["up"]["timestamp"]) {
                Canvas.camera.up.set(
                    parameters["camera"]["up"]["value"].x,
                    parameters["camera"]["up"]["value"].y,
                    parameters["camera"]["up"]["value"].z);
                Canvas.lastCameraUp["value"] = Canvas.camera.up.clone();
                Canvas.lastCameraUp["timestamp"] = parameters["camera"]["up"]["timestamp"];
                needUpdate = true;
            }
        }
        if ("zoom" in parameters["camera"]) {
            if (Canvas.lastCameraZoom["timestamp"] <= parameters["camera"]["zoom"]["timestamp"]) {
                Canvas.camera.zoom = parameters["camera"]["zoom"]["value"];
                Canvas.lastCameraZoom["value"] = Canvas.camera.zoom;
                Canvas.lastCameraZoom["timestamp"] = parameters["camera"]["zoom"]["timestamp"];
                needUpdate = true;
            }
        }
    }

    if (needUpdate) {
        Canvas.camera.updateProjectionMatrix();
        Canvas.camera.lookAt(Canvas.controls.target.clone());
        Canvas.camera.updateMatrixWorld(true);
    }
}

export const init = async function () {
    // update strokeTimeStamp on pointerDown
    document.addEventListener("pointerdown", function (e) {
        e.preventDefault();
        MouseKey.strokeTimeStamp = Date.now();
        Canvas.lastControlTarget["timestamp"] = MouseKey.strokeTimeStamp;
        Canvas.lastCameraPosition["timestamp"] = MouseKey.strokeTimeStamp;
        Canvas.lastCameraUp["timestamp"] = MouseKey.strokeTimeStamp;
        Canvas.lastCameraZoom["timestamp"] = MouseKey.strokeTimeStamp;
    });
    document.addEventListener("wheel", function(e){
        // e.preventDefault();
        MouseKey.strokeTimeStamp = Date.now();
        Canvas.lastControlTarget["timestamp"] = MouseKey.strokeTimeStamp;
        Canvas.lastCameraPosition["timestamp"] = MouseKey.strokeTimeStamp;
        Canvas.lastCameraUp["timestamp"] = MouseKey.strokeTimeStamp;
        Canvas.lastCameraZoom["timestamp"] = MouseKey.strokeTimeStamp;
    });

    WSTasks["pushCanvasParameters"] = pushCanvasParameters;
}

