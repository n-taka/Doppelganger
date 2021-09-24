import * as THREE from 'https://unpkg.com/three@0.126.0/build/three.module.js';
import { TrackballControls } from 'https://unpkg.com/three@0.126.0/examples/jsm/controls/TrackballControls.js';
import { RenderPass } from 'https://unpkg.com/three@0.126.0/examples/jsm/postprocessing/RenderPass.js';
import { EffectComposer } from 'https://unpkg.com/three@0.126.0/examples/jsm/postprocessing/EffectComposer.js';
import { BufferGeometryUtils } from 'https://unpkg.com/three@0.126.0/examples/jsm/utils/BufferGeometryUtils.js';

import { UI } from './UI.js';
import { request } from './request.js';
import { MouseKey } from './MouseKey.js';

////
// [note]
// The parameters are tweaked to work well
//   with the model whose bounding box is (100, 100, 100)
////

export const Canvas = {};

Canvas.init = async function () {
    // default color
    Canvas.defaultColor = [208.0 / 255.0, 208.0 / 255.0, 208.0 / 255.0];

    // width / height
    {
        Canvas.width = UI.webGLDiv.offsetWidth;
        Canvas.height = UI.webGLDiv.offsetHeight;
    }

    // scene
    Canvas.scene = new THREE.Scene();

    // groups
    {
        Canvas.predefGroup = new THREE.Group();
        Canvas.meshGroup = new THREE.Group();
        Canvas.scene.add(Canvas.predefGroup);
        Canvas.scene.add(Canvas.meshGroup);
    }

    // orthographic camera
    {
        Canvas.camera = new THREE.OrthographicCamera(-Canvas.width / 2.0, Canvas.width / 2.0, Canvas.height / 2.0, -Canvas.height / 2.0, 0.0, 2000.0);
        Canvas.predefGroup.add(Canvas.camera);
    }

    // light (need to tweak??)
    {
        Canvas.ambientLight = new THREE.AmbientLight(0x444444);
        Canvas.predefGroup.add(Canvas.ambientLight);
        Canvas.directionalLights = [];
        Canvas.directionalLights.push(new THREE.DirectionalLight(0xaaaaaa));
        Canvas.directionalLights[0].shadow.mapSize.width = 2048;
        Canvas.directionalLights[0].shadow.mapSize.height = 2048;
        Canvas.directionalLights[0].position.set(20, 20, 20);
        Canvas.directionalLights[0].castShadow = true;
        Canvas.predefGroup.add(Canvas.directionalLights[0]);
        Canvas.directionalLights.push(new THREE.DirectionalLight(0xaaaaaa));
        Canvas.directionalLights[1].shadow.mapSize.width = 2048;
        Canvas.directionalLights[1].shadow.mapSize.height = 2048;
        Canvas.directionalLights[1].position.set(0, -15, -20);
        Canvas.directionalLights[1].castShadow = true;
        Canvas.predefGroup.add(Canvas.directionalLights[1]);
        Canvas.directionalLights.push(new THREE.DirectionalLight(0xaaaaaa));
        Canvas.directionalLights[2].shadow.mapSize.width = 2048;
        Canvas.directionalLights[2].shadow.mapSize.height = 2048;
        Canvas.directionalLights[2].position.set(-25, 20, 0);
        Canvas.directionalLights[2].castShadow = true;
        Canvas.predefGroup.add(Canvas.directionalLights[2]);
    }

    // renderer
    {
        Canvas.renderer = new THREE.WebGLRenderer({ preserveDrawingBuffer: true, alpha: true });
        Canvas.renderer.setSize(Canvas.width, Canvas.height);
        Canvas.renderer.setClearColor(0xeeeeee, 1.0);
        Canvas.renderer.shadowMap.enabled = true;
        UI.webGLOutputDiv.appendChild(this.renderer.domElement);
    }

    // effectComposer
    {
        Canvas.effectComposer = new EffectComposer(Canvas.renderer);
        Canvas.renderPass = new RenderPass(Canvas.scene, Canvas.camera);
        Canvas.effectComposer.addPass(Canvas.renderPass);
    }

    // controls
    {
        Canvas.controls = new TrackballControls(Canvas.camera, Canvas.renderer.domElement);
        Canvas.controls.panSpeed = 0.3 * 5;
    }

    // event listener
    window.addEventListener('resize', function () {
        Canvas.width = UI.webGLDiv.offsetWidth;
        Canvas.height = UI.webGLDiv.offsetHeight;

        Canvas.camera.left = -Canvas.width / 2.0;
        Canvas.camera.right = Canvas.width / 2.0;
        Canvas.camera.top = Canvas.height / 2.0;
        Canvas.camera.bottom = -Canvas.height / 2.0;
        Canvas.camera.updateProjectionMatrix();

        Canvas.renderer.setPixelRatio(window.devicePixelRatio);
        Canvas.renderer.setSize(Canvas.width, Canvas.height);
        Canvas.effectComposer.setSize(Canvas.width, Canvas.height);
    });

    // todo
    await Canvas.pullUpdate();

    Canvas.drawLoop();
    return;
};

Canvas.drawLoop = function () {
    Canvas.controls.update();
    // todo
    //   canvas.pushUpdate();
    requestAnimationFrame(Canvas.drawLoop);
    Canvas.effectComposer.render();
};

Canvas.pullUpdate = async function () {
    const response = await request("pullCanvasParameters", {});
    const json = JSON.parse(response);
    Canvas.controls.target.set(json["controls"]["target"].x, json["controls"]["target"].y, json["controls"]["target"].z);
    Canvas.camera.position.set(json["camera"]["position"].x, json["camera"]["position"].y, json["camera"]["position"].z);
    Canvas.camera.up.set(json["camera"]["up"].x, json["camera"]["up"].y, json["camera"]["up"].z);
    Canvas.camera.zoom = json["camera"]["zoom"];
    Canvas.camera.updateProjectionMatrix();
    Canvas.camera.lookAt(Canvas.controls.target.clone());
    Canvas.camera.updateMatrixWorld(true);

    // for avoiding (semi-)infinite messaging.
    Canvas.lastControlTarget = Canvas.controls.target.clone();
    Canvas.lastCameraPos = Canvas.camera.position.clone();
    Canvas.lastCameraUp = Canvas.camera.up.clone();
    Canvas.lastCameraZoom = Canvas.camera.zoom;
    Canvas.lastCursorDir = new THREE.Vector2(0, 0);

    // cursor
    for (let sessionUUID in json["cursors"]) {
        if (!MouseKey["cursors"][sessionUUID]) {
            // new entry
            MouseKey["cursors"][sessionUUID] = { "dir": new THREE.Vector2(0, 0), "idx": json["cursors"][sessionUUID]["idx"], "img": new Image() };
            const style = MouseKey["cursors"][sessionUUID].img.style;
            style.position = "fixed";
            style["z-index"] = "1000"; // material css sidenav has 999
            style["pointer-events"] = "none";
            MouseKey["cursors"][sessionUUID].img.src = "../icon/cursorIcon" + (MouseKey["cursors"][sessionUUID].idx % 10) + ".png";

            document.body.appendChild(MouseKey["cursors"][sessionUUID].img);
        }
        MouseKey["cursors"][sessionUUID]["dir"].set(json["cursors"][sessionUUID]["dir"].x, json["cursors"][sessionUUID]["dir"].y);

        const vector = MouseKey["cursors"][sessionUUID]["dir"].clone();
        const clientX = vector.x + window.innerWidth / 2.0;
        const clientY = vector.y + window.innerHeight / 2.0;
        MouseKey["cursors"][sessionUUID].img.style.left = (clientX - 16) + "px";
        MouseKey["cursors"][sessionUUID].img.style.top = (clientY - 16) + "px";
    }
};

Canvas.pushUpdate = function () {
    //     var targetnEq = (canvas.lastControlTarget != null
    //         && !canvas.lastControlTarget.equals(canvas.controls.target));
    //     var posnEq = (canvas.lastCameraPos != null
    //         && !canvas.lastCameraPos.equals(canvas.camera.position));
    //     var upnEq = (canvas.lastCameraUp != null
    //         && !canvas.lastCameraUp.equals(canvas.camera.up));
    //     var zoomnEq = (canvas.lastCameraZoom != null
    //         && canvas.lastCameraZoom != canvas.camera.zoom);

    //     var jsonObj = {
    //         "sessionId": DoppelCore.sessionId,
    //         "timestamp": DoppelCore.strokeTimeStamp,
    //         "task": "syncParams"
    //     };
    //     if (targetnEq) {
    //         jsonObj["target"] = canvas.controls.target;
    //         canvas.lastControlTarget = canvas.controls.target.clone();
    //     }
    //     if (posnEq) {
    //         jsonObj["pos"] = canvas.camera.position;
    //         canvas.lastCameraPos = canvas.camera.position.clone();
    //     }
    //     if (upnEq) {
    //         jsonObj["up"] = canvas.camera.up;
    //         canvas.lastCameraUp = canvas.camera.up.clone();
    //     }
    //     if (zoomnEq) {
    //         jsonObj["zoom"] = canvas.camera.zoom;
    //         canvas.lastCameraZoom = canvas.camera.zoom;
    //     }

    //     if (targetnEq || posnEq || upnEq || zoomnEq) {
    //         // this is redundant, but this improves UX!

    //         var msg = JSON.stringify(jsonObj);
    //         DoppelWS.sendMsg(msg);
    //     }
};

Canvas.resetCamera = function (refreshBSphere) {
    // export function resetCamera(refreshBSphere) {
    //     if (canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh); }).length > 0) {
    //         var sliderValue = document.getElementById("clippingSlider").noUiSlider.get();
    //         var clippingNear = (parseFloat(sliderValue[0]) - 50) / 50;
    //         var clippingFar = (parseFloat(sliderValue[1]) - 50) / 50;

    //         if (refreshBSphere) {
    //             var posAttrib = BufferGeometryUtils.mergeBufferAttributes(canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh); }).map(function (obj) { return obj.geometry.getAttribute("position"); }));
    //             var geometry = new THREE.BufferGeometry();
    //             geometry.setAttribute("position", posAttrib);
    //             geometry.computeBoundingSphere();
    //             canvas.unifiedBSphere = geometry.boundingSphere.clone();
    //             geometry.dispose();
    //         }

    //         var targetToCamera = canvas.camera.position.clone();
    //         targetToCamera.sub(canvas.controls.target);
    //         targetToCamera.normalize();

    //         var targetToBCenter = canvas.unifiedBSphere.center.clone();
    //         targetToBCenter.sub(canvas.controls.target);
    //         var shift = targetToBCenter.length();
    //         shift += canvas.unifiedBSphere.radius;
    //         // for safety
    //         shift *= 1.01;
    //         var cameraPos = canvas.controls.target.clone();
    //         targetToCamera.multiplyScalar(shift);
    //         cameraPos.add(targetToCamera);
    //         canvas.camera.position.copy(cameraPos);

    //         // update pan speed
    //         targetToCamera = canvas.camera.position.clone();
    //         targetToCamera.sub(canvas.controls.target);
    //         canvas.controls.panSpeed = 100.0 / targetToCamera.length();

    //         // update near/far clip
    //         var cameraToBCenter = canvas.unifiedBSphere.center.clone();
    //         cameraToBCenter.sub(canvas.camera.position);
    //         var cameraToTarget = canvas.controls.target.clone();
    //         cameraToTarget.sub(canvas.camera.position);
    //         cameraToTarget.normalize();
    //         canvas.camera.near = cameraToBCenter.dot(cameraToTarget) + canvas.unifiedBSphere.radius * clippingNear
    //         canvas.camera.far = cameraToBCenter.dot(cameraToTarget) + canvas.unifiedBSphere.radius * clippingFar
    //         canvas.camera.updateProjectionMatrix();

    //         DoppelCore.strokeTimeStamp = Date.now();
    //         syncCanvasParameters();
    //     }
    // }
};

Canvas.alignCamera = function () {
    // export function alignCamera(direction) {
    //     // directions[X][0]: camera position based from the origin
    //     // directions[X][1]: camera up direction
    //     var directions = [
    //         [new THREE.Vector3(0, 0, 1), new THREE.Vector3(0, 1, 0)],
    //         [new THREE.Vector3(-1, 0, 0), new THREE.Vector3(0, 1, 0)],
    //         [new THREE.Vector3(0, 1, 0), new THREE.Vector3(0, 0, -1)],
    //         [new THREE.Vector3(0, 0, -1), new THREE.Vector3(0, 1, 0)],
    //         [new THREE.Vector3(1, 0, 0), new THREE.Vector3(0, 1, 0)],
    //         [new THREE.Vector3(0, -1, 0), new THREE.Vector3(0, 0, 1)]
    //     ];

    //     if (canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh); }).length > 0) {
    //         var posAttrib = BufferGeometryUtils.mergeBufferAttributes(canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh); }).map(function (obj) { return obj.geometry.getAttribute("position"); }));
    //         var geometry = new THREE.BufferGeometry();
    //         geometry.setAttribute("position", posAttrib);
    //         geometry.computeBoundingSphere();
    //         var BSphere = geometry.boundingSphere;

    //         canvas.controls.target.copy(BSphere.center);

    //         var targetToCamera = directions[direction][0].clone();
    //         targetToCamera.multiplyScalar(BSphere.radius * 1.05);

    //         canvas.camera.position.copy(targetToCamera);
    //         canvas.camera.position.add(canvas.controls.target);
    //         canvas.camera.far = targetToCamera.length() * 2.0;

    //         canvas.camera.up.copy(directions[direction][1]);

    //         // update pan speed
    //         targetToCamera = canvas.camera.position.clone();
    //         targetToCamera.sub(canvas.controls.target);
    //         canvas.controls.panSpeed = 100.0 / targetToCamera.length();

    //         canvas.camera.far = targetToCamera.length() * 2.0;
    //         canvas.camera.updateProjectionMatrix();

    //         DoppelCore.strokeTimeStamp = Date.now();
    //         syncCanvasParameters();
    //     } else {
    //         canvas.controls.target.copy(new THREE.Vector3(0, 0, 0));
    //         canvas.camera.position.copy(directions[direction][0]);
    //         canvas.camera.up.copy(directions[direction][1]);
    //         canvas.camera.updateProjectionMatrix();
    //         canvas.controls.panSpeed = 100.0;

    //         DoppelCore.strokeTimeStamp = Date.now();
    //         syncCanvasParameters();
    //     }
    // }
};

Canvas.fitToFrame = function () {
    // export function fitToFrame() {
    //     if (canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh && obj.visible); }).length > 0) {
    //         var posAttrib = BufferGeometryUtils.mergeBufferAttributes(canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh && obj.visible); }).map(function (obj) { return obj.geometry.getAttribute("position"); }));
    //         var geometry = new THREE.BufferGeometry();
    //         geometry.setAttribute("position", posAttrib);
    //         geometry.computeBoundingSphere();
    //         var BSphere = geometry.boundingSphere;

    //         var translateVec = BSphere.center.clone();
    //         translateVec.sub(canvas.controls.target);
    //         canvas.camera.position.add(translateVec);
    //         canvas.controls.target.add(translateVec);

    //         // update pan speed
    //         var targetToCamera = canvas.camera.position.clone();
    //         targetToCamera.sub(canvas.controls.target);
    //         canvas.controls.panSpeed = 100 / targetToCamera.length();

    //         // var raycaster = new THREE.Raycaster();
    //         // var mouse = new THREE.Vector2();
    //         // mouse.x = (window.innerWidth > window.innerHeight) ? 0.0 : 1.0;
    //         // mouse.y = (window.innerWidth > window.innerHeight) ? 1.0 : 0.0;
    //         // raycaster.setFromCamera(mouse, canvas.camera);

    //         // var theta = raycaster.ray.direction.angleTo(targetToCamera);
    //         // var distTargetToCamera = BSphere.radius / Math.sin(theta);

    //         // canvas.camera.position.copy(canvas.controls.target);
    //         // var offset = targetToCamera.multiplyScalar(distTargetToCamera / targetToCamera.length());
    //         // canvas.camera.position.add(offset);

    //         if (canvas.width > canvas.height) {
    //             canvas.camera.zoom = (canvas.height / 2) / BSphere.radius;
    //         } else {
    //             canvas.camera.zoom = (canvas.width / 2) / BSphere.radius;
    //         }
    //         canvas.camera.far = targetToCamera.length() * 2.0;
    //         canvas.camera.updateProjectionMatrix();

    //         DoppelCore.strokeTimeStamp = Date.now();
    //         syncCanvasParameters();
    //     }
    // }
};
