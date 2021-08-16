import * as THREE from 'https://unpkg.com/three@0.126.0/build/three.module.js';
import { TrackballControls } from 'https://unpkg.com/three@0.126.0/examples/jsm/controls/TrackballControls.js';
import { RenderPass } from 'https://unpkg.com/three@0.126.0/examples/jsm/postprocessing/RenderPass.js';
import { EffectComposer } from 'https://unpkg.com/three@0.126.0/examples/jsm/postprocessing/EffectComposer.js';
import { BufferGeometryUtils } from 'https://unpkg.com/three@0.126.0/examples/jsm/utils/BufferGeometryUtils.js';
import { DoppelWS } from './websocket.js';
import { APIcall } from './APIcall.js';
import { DoppelCore } from './DoppelCore.js';
import { mouseCursors } from './mouseKey.js';

////
// [note]
// The parameters are tweaked to work well
//   with the model whose bounding box is (100, 100, 100)
////
var targetCanvas = function (w, h) {
    this.width = document.getElementById("WebGLDiv").offsetWidth;
    this.height = document.getElementsByTagName("body")[0].offsetHeight;
    this.scene = null;
    this.camera = null;
    this.lastControlTarget = null;
    this.lastCameraPos = null;
    this.lastCameraUp = null;
    this.lastCameraZoom = null;
    this.lastCursorDir = null;
    this.predefGroup = null;
    this.meshGroup = null;
    this.ambientLight = null;
    this.light = [];
    this.controls = null;
    this.renderer = null;
    this.effectComposer = null;
    this.renderPass = null;
    this.animFunc = null;
    this.defaultColor = [208.0 / 255.0, 208.0 / 255.0, 208.0 / 255.0];
    this.mouseCursors = [];
}

export var canvas = new targetCanvas();

targetCanvas.prototype.init = function () {
    this.scene = new THREE.Scene();

    // perspective
    // this.camera = new THREE.PerspectiveCamera(45, this.width / this.height, 0.01, 1000);

    // orthographic
    this.camera = new THREE.OrthographicCamera(-this.width / 2.0, this.width / 2.0, this.height / 2.0, -this.height / 2.0, 0.0, 2000.0);

    this.predefGroup = new THREE.Group();
    this.meshGroup = new THREE.Group();
    this.backMeshGroup = new THREE.Group();

    this.predefGroup.add(this.camera);

    this.scene.add(this.predefGroup);
    this.scene.add(this.meshGroup);
    this.scene.add(this.backMeshGroup);

    // create a render and set the size
    this.renderer = new THREE.WebGLRenderer({ preserveDrawingBuffer: true, alpha: true });
    this.renderer.setSize(this.width, this.height);
    this.renderer.setClearColor(0xeeeeee, 1.0);
    this.renderer.shadowMap.enabled = true;

    document.getElementById("WebGL-output").appendChild(this.renderer.domElement);
    this.controls = new TrackballControls(this.camera, this.renderer.domElement);
    this.controls.panSpeed = 0.3 * 5;

    this.renderPass = new RenderPass(this.scene, this.camera);

    this.effectComposer = new EffectComposer(this.renderer);
    this.effectComposer.addPass(this.renderPass);

    this.ambientLight = new THREE.AmbientLight(0x444444);
    this.predefGroup.add(this.ambientLight);
    this.light.push(new THREE.DirectionalLight(0xaaaaaa));
    this.light[0].shadow.mapSize.width = 2048;
    this.light[0].shadow.mapSize.height = 2048;
    this.predefGroup.add(this.light[0]);
    this.light.push(new THREE.DirectionalLight(0xaaaaaa));
    this.light[1].shadow.mapSize.width = 2048;
    this.light[1].shadow.mapSize.height = 2048;
    this.predefGroup.add(this.light[1]);
    this.light.push(new THREE.DirectionalLight(0xaaaaaa));
    this.light[2].shadow.mapSize.width = 2048;
    this.light[2].shadow.mapSize.height = 2048;
    this.predefGroup.add(this.light[2]);

    // this.stats = new Stats();
    // this.stats.setMode(0);
    // document.getElementById("Stats-output").appendChild(this.stats.domElement);

    this.initScene();
    this.drawLoop();
};

targetCanvas.prototype.initScene = function () {
    getCanvasParameters();
    this.camera.lookAt(this.scene.position);
    // add spotlight for the shadows
    this.light[0].position.set(20, 20, 20);
    this.light[0].castShadow = true;
    this.light[1].position.set(0, -15, -20);
    this.light[1].castShadow = true;
    this.light[2].position.set(-25, 20, 0);
    this.light[2].castShadow = true;

    this.animFunc = function () {
    }
}

canvas.drawLoop = function () {
    canvas.controls.update();
    syncCanvasParameters();
    // canvas.stats.update();
    canvas.animFunc();
    requestAnimationFrame(canvas.drawLoop);
    canvas.effectComposer.render();
    // canvas.renderer.render(canvas.scene, canvas.camera);
};

export function resetCamera(refreshBSphere) {
    if (canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh); }).length > 0) {
        var sliderValue = document.getElementById("clippingSlider").noUiSlider.get();
        var clippingNear = (parseFloat(sliderValue[0]) - 50) / 50;
        var clippingFar = (parseFloat(sliderValue[1]) - 50) / 50;

        if (refreshBSphere) {
            var posAttrib = BufferGeometryUtils.mergeBufferAttributes(canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh); }).map(function (obj) { return obj.geometry.getAttribute("position"); }));
            var geometry = new THREE.BufferGeometry();
            geometry.setAttribute("position", posAttrib);
            geometry.computeBoundingSphere();
            canvas.unifiedBSphere = geometry.boundingSphere.clone();
            geometry.dispose();
        }

        var targetToCamera = canvas.camera.position.clone();
        targetToCamera.sub(canvas.controls.target);
        targetToCamera.normalize();

        var targetToBCenter = canvas.unifiedBSphere.center.clone();
        targetToBCenter.sub(canvas.controls.target);
        var shift = targetToBCenter.length();
        shift += canvas.unifiedBSphere.radius;
        // for safety
        shift *= 1.01;
        var cameraPos = canvas.controls.target.clone();
        targetToCamera.multiplyScalar(shift);
        cameraPos.add(targetToCamera);
        canvas.camera.position.copy(cameraPos);

        // update pan speed
        targetToCamera = canvas.camera.position.clone();
        targetToCamera.sub(canvas.controls.target);
        canvas.controls.panSpeed = 100.0 / targetToCamera.length();

        // update near/far clip
        var cameraToBCenter = canvas.unifiedBSphere.center.clone();
        cameraToBCenter.sub(canvas.camera.position);
        var cameraToTarget = canvas.controls.target.clone();
        cameraToTarget.sub(canvas.camera.position);
        cameraToTarget.normalize();
        canvas.camera.near = cameraToBCenter.dot(cameraToTarget) + canvas.unifiedBSphere.radius * clippingNear
        canvas.camera.far = cameraToBCenter.dot(cameraToTarget) + canvas.unifiedBSphere.radius * clippingFar
        canvas.camera.updateProjectionMatrix();

        DoppelCore.strokeTimeStamp = Date.now();
        syncCanvasParameters();
    }
}

export function alignCamera(direction) {
    // directions[X][0]: camera position based from the origin
    // directions[X][1]: camera up direction
    var directions = [
        [new THREE.Vector3(0, 0, 1), new THREE.Vector3(0, 1, 0)],
        [new THREE.Vector3(-1, 0, 0), new THREE.Vector3(0, 1, 0)],
        [new THREE.Vector3(0, 1, 0), new THREE.Vector3(0, 0, -1)],
        [new THREE.Vector3(0, 0, -1), new THREE.Vector3(0, 1, 0)],
        [new THREE.Vector3(1, 0, 0), new THREE.Vector3(0, 1, 0)],
        [new THREE.Vector3(0, -1, 0), new THREE.Vector3(0, 0, 1)]
    ];

    if (canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh); }).length > 0) {
        var posAttrib = BufferGeometryUtils.mergeBufferAttributes(canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh); }).map(function (obj) { return obj.geometry.getAttribute("position"); }));
        var geometry = new THREE.BufferGeometry();
        geometry.setAttribute("position", posAttrib);
        geometry.computeBoundingSphere();
        var BSphere = geometry.boundingSphere;

        canvas.controls.target.copy(BSphere.center);

        var targetToCamera = directions[direction][0].clone();
        targetToCamera.multiplyScalar(BSphere.radius * 1.05);

        canvas.camera.position.copy(targetToCamera);
        canvas.camera.position.add(canvas.controls.target);
        canvas.camera.far = targetToCamera.length() * 2.0;

        canvas.camera.up.copy(directions[direction][1]);

        // update pan speed
        targetToCamera = canvas.camera.position.clone();
        targetToCamera.sub(canvas.controls.target);
        canvas.controls.panSpeed = 100.0 / targetToCamera.length();

        canvas.camera.far = targetToCamera.length() * 2.0;
        canvas.camera.updateProjectionMatrix();

        DoppelCore.strokeTimeStamp = Date.now();
        syncCanvasParameters();
    } else {
        canvas.controls.target.copy(new THREE.Vector3(0, 0, 0));
        canvas.camera.position.copy(directions[direction][0]);
        canvas.camera.up.copy(directions[direction][1]);
        canvas.camera.updateProjectionMatrix();
        canvas.controls.panSpeed = 100.0;

        DoppelCore.strokeTimeStamp = Date.now();
        syncCanvasParameters();
    }
}

export function fitToFrame() {
    if (canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh && obj.visible); }).length > 0) {
        var posAttrib = BufferGeometryUtils.mergeBufferAttributes(canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh && obj.visible); }).map(function (obj) { return obj.geometry.getAttribute("position"); }));
        var geometry = new THREE.BufferGeometry();
        geometry.setAttribute("position", posAttrib);
        geometry.computeBoundingSphere();
        var BSphere = geometry.boundingSphere;

        var translateVec = BSphere.center.clone();
        translateVec.sub(canvas.controls.target);
        canvas.camera.position.add(translateVec);
        canvas.controls.target.add(translateVec);

        // update pan speed
        var targetToCamera = canvas.camera.position.clone();
        targetToCamera.sub(canvas.controls.target);
        canvas.controls.panSpeed = 100 / targetToCamera.length();

        // var raycaster = new THREE.Raycaster();
        // var mouse = new THREE.Vector2();
        // mouse.x = (window.innerWidth > window.innerHeight) ? 0.0 : 1.0;
        // mouse.y = (window.innerWidth > window.innerHeight) ? 1.0 : 0.0;
        // raycaster.setFromCamera(mouse, canvas.camera);

        // var theta = raycaster.ray.direction.angleTo(targetToCamera);
        // var distTargetToCamera = BSphere.radius / Math.sin(theta);

        // canvas.camera.position.copy(canvas.controls.target);
        // var offset = targetToCamera.multiplyScalar(distTargetToCamera / targetToCamera.length());
        // canvas.camera.position.add(offset);

        if (canvas.width > canvas.height) {
            canvas.camera.zoom = (canvas.height / 2) / BSphere.radius;
        } else {
            canvas.camera.zoom = (canvas.width / 2) / BSphere.radius;
        }
        canvas.camera.far = targetToCamera.length() * 2.0;
        canvas.camera.updateProjectionMatrix();

        DoppelCore.strokeTimeStamp = Date.now();
        syncCanvasParameters();
    }
}



function getCanvasParameters() {
    var callback = function (response) {
        var j = JSON.parse(response);
        canvas.controls.target.set(j["target"].x, j["target"].y, j["target"].z);
        canvas.camera.position.set(j["pos"].x, j["pos"].y, j["pos"].z);
        canvas.camera.up.set(j["up"].x, j["up"].y, j["up"].z);
        canvas.camera.zoom = j["zoom"];
        canvas.camera.updateProjectionMatrix();
        canvas.camera.lookAt(canvas.controls.target.clone());
        canvas.camera.updateMatrixWorld(true);

        // for avoiding (semi-)infinite messaging.
        canvas.lastControlTarget = canvas.controls.target.clone();
        canvas.lastCameraPos = canvas.camera.position.clone();
        canvas.lastCameraUp = canvas.camera.up.clone();
        canvas.lastCameraZoom = canvas.camera.zoom;
        canvas.lastCursorDir = new THREE.Vector3(0, 0, 0);

        // cursor
        for (var sessionId in j["cursor"]) {
            if (mouseCursors[sessionId] == null) {
                // new entry
                mouseCursors[sessionId] = { "dir": new THREE.Vector3(0, 0, 0), "img": new Image() };
                var style = mouseCursors[sessionId].img.style;
                style.position = "fixed";
                style["z-index"] = "1000"; // material css sidenav has 999
                style["pointer-events"] = "none";
                mouseCursors[sessionId].img.src = "../icon/cursorIcon" + (sessionId % 10) + ".png";
                mouseCursors[sessionId].img.sessionId = sessionId;

                document.body.appendChild(mouseCursors[sessionId].img);
            }
            mouseCursors[sessionId]["dir"].set(j["cursor"][sessionId]["dirX"], j["cursor"][sessionId]["dirY"]);

            var vector = mouseCursors[sessionId]["dir"].clone();
            vector.multiplyScalar(1000.0);
            vector.add(canvas.lastCameraPos);
            var widthHalf = (canvas.width / 2);
            var heightHalf = (canvas.height / 2);
            // vector.project(canvas.camera);

            var clientX = (vector.x * widthHalf) + widthHalf;
            var clientY = - (vector.y * heightHalf) + heightHalf;
            mouseCursors[sessionId].img.style.left = (clientX - 16) + "px";
            mouseCursors[sessionId].img.style.top = (clientY - 16) + "px";
        }
    };
    APIcall("GET", "api/latestCanvasParameters").then(res => callback(res));
}

function syncCanvasParameters() {
    var targetnEq = (canvas.lastControlTarget != null
        && !canvas.lastControlTarget.equals(canvas.controls.target));
    var posnEq = (canvas.lastCameraPos != null
        && !canvas.lastCameraPos.equals(canvas.camera.position));
    var upnEq = (canvas.lastCameraUp != null
        && !canvas.lastCameraUp.equals(canvas.camera.up));
    var zoomnEq = (canvas.lastCameraZoom != null
        && canvas.lastCameraZoom != canvas.camera.zoom);

    var jsonObj = {
        "sessionId": DoppelCore.sessionId,
        "timestamp": DoppelCore.strokeTimeStamp,
        "task": "syncParams"
    };
    if (targetnEq) {
        jsonObj["target"] = canvas.controls.target;
        canvas.lastControlTarget = canvas.controls.target.clone();
    }
    if (posnEq) {
        jsonObj["pos"] = canvas.camera.position;
        canvas.lastCameraPos = canvas.camera.position.clone();
    }
    if (upnEq) {
        jsonObj["up"] = canvas.camera.up;
        canvas.lastCameraUp = canvas.camera.up.clone();
    }
    if (zoomnEq) {
        jsonObj["zoom"] = canvas.camera.zoom;
        canvas.lastCameraZoom = canvas.camera.zoom;
    }

    if (targetnEq || posnEq || upnEq || zoomnEq) {
        // this is redundant, but this improves UX!

        var msg = JSON.stringify(jsonObj);
        DoppelWS.sendMsg(msg);
    }
}
