import * as THREE from 'https://cdn.skypack.dev/three@v0.132';
import { Canvas } from '../../js/Canvas.js';
import { UI } from '../../js/UI.js';

const fitToFrame = function () {
    const visibleMeshList = Canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh && obj.visible); });
    if (visibleMeshList.length > 0) {
        // for updating Canvas.unifiedBSphere
        Canvas.resetCamera(true);
        // change camera position, zoom
        const translateVec = Canvas.unifiedBSphere.center.clone();
        translateVec.sub(Canvas.controls.target);
        Canvas.camera.position.add(translateVec);
        Canvas.controls.target.add(translateVec);
        if (Canvas.width > Canvas.height) {
            Canvas.camera.zoom = (Canvas.height * 0.5) / Canvas.unifiedBSphere.radius;
        } else {
            Canvas.camera.zoom = (Canvas.width * 0.5) / Canvas.unifiedBSphere.radius;
        }
        // for updating camera.clippingNear, clippingFar, making sure that whole of the mesh is visible
        Canvas.resetCamera(false);
        // Canvas.pushUpdate() is called within next drawLoop
        // Canvas.pushUpdate();
    }
}

export const init = async function () {
    Canvas.fitToFrame = fitToFrame;

    // keyboard event
    document.addEventListener("keyup", (function (e) {
        for (let modal of UI.modalDiv.children) {
            const instance = M.Modal.getInstance(modal);
            if(instance.isOpen){
                return;
            }
        }
        const keycode = e.code;
        if (keycode == 'KeyF') {
            // fitToFrame() updates MouseKey.strokeTimeStamp
            fitToFrame();
        }
    }));
}

