import * as THREE from 'https://cdn.skypack.dev/three@v0.132';
import { OutlinePass } from 'https://cdn.skypack.dev/three@v0.132/examples/jsm/postprocessing/OutlinePass.js';
import { Canvas } from '../../js/Canvas.js';
import { UI } from '../../js/UI.js';
import { constructMeshLiFromJson } from '../../js/constructMeshLiFrom.js';

const findClosestMesh = function (e) {
    // [IN]
    //  e: mouse event
    // [OUT]
    //  UUID for intersected mesh
    const mouse = new THREE.Vector2();
    const clientX = e.clientX;
    const clientY = e.clientY;
    mouse.x = (clientX / Canvas.width) * 2 - 1;
    mouse.y = -(clientY / Canvas.height) * 2 + 1;

    const raycaster = new THREE.Raycaster();
    raycaster.setFromCamera(mouse, Canvas.camera);

    const intersects = raycaster.intersectObjects(Canvas.meshGroup.children);
    if (intersects.length > 0) {
        for (let h of intersects) {
            // we only check intersection for visible parts
            if (h.object.visible) {
                return h.object.DoppelgangerUUID;
            }
        }
    }
    return "";
}

const colorizeActiveMesh = function (selectedUUID) {
    // all background are set to #ffffff
    // previous outline is removed
    for (let UUID in UI.UUIDToMeshLi) {
        const li = UI.UUIDToMeshLi[UUID];
        li.style.background = "#ffffff";
    }
    Canvas.outlinePass.selectedObjects = [];

    // enable background color for selected mesh
    // add outline for selected mesh
    if (selectedUUID in UI.UUIDToMeshLi) {
        UI.UUIDToMeshLi[selectedUUID].style.background = "#f5c7a6";
        Canvas.outlinePass.selectedObjects.push(Canvas.UUIDToMesh[selectedUUID]);
    }
}

export const init = async function () {
    if (!Canvas.outlinePass) {
        Canvas.outlinePass = new OutlinePass(new THREE.Vector2(Canvas.width, Canvas.height), Canvas.scene, Canvas.camera);
        Canvas.outlinePass.edgeStrength = 3.0;
        Canvas.outlinePass.edgeGlow = 0.0;
        Canvas.outlinePass.edgeThickness = 1.0;
        Canvas.outlinePass.pulsePeriod = 0.0;
        Canvas.outlinePass.visibleEdgeColor.set("#f56c0a");
        Canvas.outlinePass.hiddenEdgeColor.set("#0a93f5");
        // because our default color (background, mesh) are almost white
        Canvas.outlinePass.overlayMaterial.blending = THREE.CustomBlending;
        Canvas.effectComposer.addPass(Canvas.outlinePass);
    }

    Canvas.controls.domElement.addEventListener("pointerdown", function (e) {
        const closestMeshUUID = findClosestMesh(e);
        colorizeActiveMesh(closestMeshUUID);
    });

    constructMeshLiFromJson.handlers.push(
        function (json, liRoot) {
            liRoot.addEventListener('click', function () {
                colorizeActiveMesh(json["UUID"]);
            });
        }
    );
}
