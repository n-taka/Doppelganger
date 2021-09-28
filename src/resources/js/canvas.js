import * as THREE from 'https://cdn.skypack.dev/three';
import { TrackballControls } from 'https://cdn.skypack.dev/three/examples/jsm/controls/TrackballControls.js';
import { mergeBufferAttributes } from 'https://cdn.skypack.dev/three/examples/jsm/utils/BufferGeometryUtils.js';
import { RenderPass } from 'https://cdn.skypack.dev/three/examples/jsm/postprocessing/RenderPass.js';
import { EffectComposer } from 'https://cdn.skypack.dev/three/examples/jsm/postprocessing/EffectComposer.js';

import { UI } from './UI.js';
import { Core } from './Core.js';
import { WS } from './WS.js';
import { request } from './request.js';
import { MouseKey } from './MouseKey.js';

import { constructMeshFromParameters } from './constructMeshFrom.js';
import { constructMeshLiFromParameters } from './constructMeshLiFrom.js';

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

    // bounding sphere (for tweaking camera parameters)
    Canvas.unifiedBSphere = new THREE.Sphere(new THREE.Vector3(0, 0, 0), 1.0);

    // parameters for UI
    Canvas.lastControlTarget = {};
    Canvas.lastCameraPosition = {};
    Canvas.lastCameraUp = {};
    Canvas.lastCameraZoom = {};
    Canvas.lastControlTarget["value"] = new THREE.Vector3(0, 0, 0);
    Canvas.lastControlTarget["timestamp"] = -1;
    Canvas.lastCameraPosition["value"] = new THREE.Vector3(0, 0, 0);
    Canvas.lastCameraPosition["timestamp"] = -1;
    Canvas.lastCameraUp["value"] = new THREE.Vector3(0, 0, 0);
    Canvas.lastCameraUp["timestamp"] = -1;
    Canvas.lastCameraZoom["value"] = 1.0;
    Canvas.lastCameraZoom["timestamp"] = -1;

    // parameter for mesh
    Canvas.UUIDToMesh = {};

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

    await Canvas.pullCanvasParameters();

    Canvas.drawLoop();
    return;
};

Canvas.drawLoop = function () {
    Canvas.controls.update();
    Canvas.pushCanvasParameters();
    requestAnimationFrame(Canvas.drawLoop);
    Canvas.effectComposer.render();
};

Canvas.pullCanvasParameters = async function () {
    const json = JSON.parse(await request("pullCanvasParameters", {}));
    Canvas.controls.target.set(json["controls"]["target"].x, json["controls"]["target"].y, json["controls"]["target"].z);
    Canvas.camera.position.set(json["camera"]["position"].x, json["camera"]["position"].y, json["camera"]["position"].z);
    Canvas.camera.up.set(json["camera"]["up"].x, json["camera"]["up"].y, json["camera"]["up"].z);
    Canvas.camera.zoom = json["camera"]["zoom"]["value"];
    Canvas.camera.updateProjectionMatrix();
    Canvas.camera.lookAt(Canvas.controls.target.clone());
    Canvas.camera.updateMatrixWorld(true);

    // for avoiding (semi-)infinite messaging.
    Canvas.lastControlTarget["value"] = Canvas.controls.target.clone();
    Canvas.lastControlTarget["timestamp"] = json["controls"]["target"]["timestamp"];
    Canvas.lastCameraPosition["value"] = Canvas.camera.position.clone();
    Canvas.lastCameraPosition["timestamp"] = json["camera"]["position"]["timestamp"];
    Canvas.lastCameraUp["value"] = Canvas.camera.up.clone();
    Canvas.lastCameraUp["timestamp"] = json["camera"]["up"]["timestamp"];
    Canvas.lastCameraZoom["value"] = Canvas.camera.zoom;
    Canvas.lastCameraZoom["timestamp"] = json["camera"]["zoom"]["timestamp"];

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

Canvas.pushCanvasParameters = async function () {
    if (Core["UUID"]) {
        const targetnEq = (!Canvas.lastControlTarget["value"].equals(Canvas.controls.target));
        const posnEq = (!Canvas.lastCameraPosition["value"].equals(Canvas.camera.position));
        const upnEq = (!Canvas.lastCameraUp["value"].equals(Canvas.camera.up));
        const zoomnEq = (Canvas.lastCameraZoom["value"] != Canvas.camera.zoom);

        const json = {};
        if (targetnEq) {
            json["controls"] = {};
            Canvas.lastControlTarget["value"] = Canvas.controls.target.clone();
            Canvas.lastControlTarget["timestamp"] = MouseKey.strokeTimeStamp;
            json["controls"]["target"] = Canvas.lastControlTarget;
        }
        if (posnEq) {
            json["camera"] = {};
            Canvas.lastCameraPosition["value"] = Canvas.camera.position.clone();
            Canvas.lastCameraPosition["timestamp"] = MouseKey.strokeTimeStamp;
            json["camera"]["position"] = Canvas.lastCameraPosition;
        }
        if (upnEq) {
            if (!json["camera"]) {
                json["camera"] = {};
            }
            Canvas.lastCameraUp["value"] = Canvas.camera.up.clone();
            Canvas.lastCameraUp["timestamp"] = MouseKey.strokeTimeStamp;
            json["camera"]["up"] = Canvas.lastCameraUp;
        }
        if (zoomnEq) {
            if (!json["camera"]) {
                json["camera"] = {};
            }
            Canvas.lastCameraZoom["value"] = Canvas.camera.zoom;
            Canvas.lastCameraZoom["timestamp"] = MouseKey.strokeTimeStamp;
            json["camera"]["zoom"] = Canvas.lastCameraZoom;
        }

        if (targetnEq || posnEq || upnEq || zoomnEq) {
            WS.sendMsg("pushCanvasParameters", json);
        }
    }
};

Canvas.pullCurrentMeshes = async function () {
    const parameters = JSON.parse(await request("pullCurrentMeshes"));
    constructMeshFromParameters(parameters);
    constructMeshLiFromParameters(parameters);
}

Canvas.resetCamera = function (refreshBSphere) {
    const meshList = Canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh); });
    if (meshList.length > 0) {
        // var sliderValue = document.getElementById("clippingSlider").noUiSlider.get();
        // var clippingNear = (parseFloat(sliderValue[0]) - 50) / 50;
        // var clippingFar = (parseFloat(sliderValue[1]) - 50) / 50;
        const clippingNear = -1.01;
        const clippingFar = 1.01;

        if (refreshBSphere) {
            const posAttrib = mergeBufferAttributes(meshList.map(function (obj) { return obj.geometry.getAttribute("position"); }));
            const geometry = new THREE.BufferGeometry();
            geometry.setAttribute("position", posAttrib);
            geometry.computeBoundingSphere();
            Canvas.unifiedBSphere = geometry.boundingSphere.clone();
            geometry.dispose();
        }

        // the procedure below also handles the case "controls.target != BSphere.center"
        const targetToCamera = Canvas.camera.position.clone();
        targetToCamera.sub(Canvas.controls.target);
        targetToCamera.normalize();

        const targetToBCenter = Canvas.unifiedBSphere.center.clone();
        targetToBCenter.sub(Canvas.controls.target);
        const shift = (targetToBCenter.length() + Canvas.unifiedBSphere.radius) * 1.01;
        const cameraPos = Canvas.controls.target.clone();
        targetToCamera.multiplyScalar(shift);
        cameraPos.add(targetToCamera);
        Canvas.camera.position.copy(cameraPos);

        // update pan speed
        Canvas.controls.panSpeed = 100.0 / targetToCamera.length();

        // update near/far clip
        const cameraToBCenter = Canvas.unifiedBSphere.center.clone();
        cameraToBCenter.sub(Canvas.camera.position);
        const cameraToTarget = Canvas.controls.target.clone();
        cameraToTarget.sub(Canvas.camera.position);
        cameraToTarget.normalize();
        Canvas.camera.near = cameraToBCenter.dot(cameraToTarget) + Canvas.unifiedBSphere.radius * clippingNear
        Canvas.camera.far = cameraToBCenter.dot(cameraToTarget) + Canvas.unifiedBSphere.radius * clippingFar
        Canvas.camera.updateProjectionMatrix();

        MouseKey.strokeTimeStamp = Date.now();
        // Canvas.pushUpdate() is called within next drawLoop
        // Canvas.pushUpdate();
    }
};
