import * as THREE from 'https://cdn.skypack.dev/three';
import { mergeBufferAttributes } from 'https://cdn.skypack.dev/three/examples/jsm/utils/BufferGeometryUtils.js';
import { Canvas } from '../../js/Canvas.js';
import { MouseKey } from '../../js/MouseKey.js';

const fitToFrame = function () {
    const visibleMeshList = Canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh && obj.visible); });
    if (visibleMeshList.length > 0) {
        const posAttrib = mergeBufferAttributes(visibleMeshList.map(function (obj) { return obj.geometry.getAttribute("position"); }));
        const geometry = new THREE.BufferGeometry();
        geometry.setAttribute("position", posAttrib);
        geometry.computeBoundingSphere();
        const BSphere = geometry.boundingSphere;

        const translateVec = BSphere.center.clone();
        translateVec.sub(Canvas.controls.target);
        Canvas.camera.position.add(translateVec);
        Canvas.controls.target.add(translateVec);

        // update pan speed
        const targetToCamera = Canvas.camera.position.clone();
        targetToCamera.sub(Canvas.controls.target);
        Canvas.controls.panSpeed = 100 / targetToCamera.length();

        if (Canvas.width > Canvas.height) {
            Canvas.camera.zoom = (Canvas.height * 0.5) / BSphere.radius;
        } else {
            Canvas.camera.zoom = (Canvas.width * 0.5) / BSphere.radius;
        }
        Canvas.camera.far = targetToCamera.length() * 2.0;
        Canvas.camera.updateProjectionMatrix();

        MouseKey.strokeTimeStamp = Date.now();
        // Canvas.pushUpdate() is called within next drawLoop
        // Canvas.pushUpdate();
    }
}

export const init = async function () {
    Canvas.fitToFrame = fitToFrame;

    // keyboard event
    document.addEventListener("keyup", (function (e) {
        const keycode = e.code;
        if (keycode == 'KeyF') {
            // fitToFrame() updates MouseKey.strokeTimeStamp
            fitToFrame();
        }
    }));
}

