import * as THREE from 'https://unpkg.com/three@0.126.0/build/three.module.js';
import { Core } from './Core.js';
import { UI } from './UI.js';

export const WSTasks = {};

WSTasks["initializeSession"] = initializeSession;
WSTasks["isServerBusy"] = isServerBusy;
// WSTasks["syncParams"] = syncParams;
// WSTasks["syncMeshes"] = syncMeshes;

async function initializeSession(parameters) {
    ////
    // [IN]
    // parameters = {
    //  "sessionUUID": sessionUUID string
    // }
    Core.UUID = parameters["sessionUUID"];
}

async function isServerBusy(parameters) {
    ////
    // [IN]
    // parameters = {
    //  "isBusy": boolean value that represents the server is busy
    // }
    UI.setBusyMode(parameters["isBusy"]);
}

// async function updateToolElement(mesh, remove) {
//     var doppelId = mesh.doppelId;
//     var meshCollection = document.getElementById("meshCollection");

//     {
//         // explicitly destroy all tooltips (for avoiding too many zombie elements)
//         var tooltip_elems = document.querySelectorAll('.tooltipped');
//         var tooltip_instances = M.Tooltip.init(tooltip_elems, {});
//         for(var instance of tooltip_instances)
//         {
//             instance.destroy();
//         }
//     }

//     var meshCollectionFrag = document.createDocumentFragment();
//     for (var c of meshCollection.children) {
//         meshCollectionFrag.appendChild(c.cloneNode(true));

//         // manually clone handlers
//         DoppelCore.toolHandlerGenerator.forEach((generator) => {
//             if (generator.hasOwnProperty("id")) {
//                 var doppelIdSub = parseInt(c.id.substring(4));
//                 var aPlugin = meshCollectionFrag.getElementById("a" + generator.id + doppelIdSub);
//                 aPlugin.onclick = function () {
//                     DoppelCore.selectedDoppelId = [doppelIdSub];
//                     generator.handler();
//                 };
//             }
//         });

//     }

//     if (remove) {
//         var liRoot = meshCollectionFrag.getElementById("mesh" + doppelId);
//         meshCollectionFrag.removeChild(liRoot);
//     } else {
//         // root
//         var liRoot = meshCollectionFrag.getElementById("mesh" + doppelId);
//         if (liRoot) {
//             // if there are older element, we remove it.
//             // Then, we (re-)generate such element (for easy implementation)
//             liRoot.remove();
//         }
//         liRoot = document.createElement("li");
//         liRoot.setAttribute("class", "collection-item avatar");
//         liRoot.setAttribute("id", "mesh" + doppelId);

//         {
//             // icon (empty)
//             var iIcon = document.createElement("i");
//             iIcon.setAttribute("class", "material-icons circle lighten-2");
//             iIcon.setAttribute("style", "user-select: none;");
//             iIcon.innerText = "";
//             iIcon.setAttribute("id", "icon" + doppelId);
//             liRoot.appendChild(iIcon);
//         }
//         // mesh title
//         {
//             var spanTitle = document.createElement("span");
//             liRoot.appendChild(spanTitle)
//             spanTitle.setAttribute("class", "title");
//             spanTitle.setAttribute("id", "title" + doppelId);
//             spanTitle.innerHTML = mesh.name;
//         }
//         // notification
//         {
//             var aNotify = document.createElement("a");
//             aNotify.setAttribute("class", "secondary-content");
//             aNotify.setAttribute("style", "user-select: none;");
//             aNotify.setAttribute("id", "notify" + doppelId);
//             liRoot.appendChild(aNotify);
//         }
//         {
//             // meta information
//             var divMetaInfo = document.createElement("div");
//             liRoot.appendChild(divMetaInfo);
//             divMetaInfo.setAttribute("id", "metaInfo" + doppelId);

//             var pDummy = document.createElement("p");
//             divMetaInfo.appendChild(pDummy);
//             pDummy.innerHTML = "<br>";
//         }
//         {
//             // buttons
//             var pButtons = document.createElement("p");
//             liRoot.appendChild(pButtons);
//             pButtons.setAttribute("style", "text-align: right; user-select: none;");
//             pButtons.setAttribute("id", "buttons" + doppelId);

//             DoppelCore.toolHandlerGenerator.forEach((generator) => {
//                 var aPlugin = document.createElement("a");
//                 var iPlugin = document.createElement("i");
//                 iPlugin.setAttribute("class", "material-icons teal-text text-lighten-2");
//                 iPlugin.innerHTML = generator.icon;
//                 if (generator.hasOwnProperty("id")) {
//                     aPlugin.setAttribute("id", "a" + generator.id + doppelId);
//                     iPlugin.setAttribute("id", "i" + generator.id + doppelId);
//                 }
//                 aPlugin.appendChild(iPlugin);
//                 aPlugin.onclick = function () {
//                     DoppelCore.selectedDoppelId = [doppelId];
//                     generator.handler();
//                 };
//                 if (generator.hasOwnProperty("tooltip")) {
//                     aPlugin.setAttribute("class", "tooltipped");
//                     aPlugin.setAttribute("data-position", "top");
//                     aPlugin.setAttribute("data-tooltip", generator["tooltip"]);
//                 }

//                 pButtons.appendChild(aPlugin);
//             });
//         }

//         meshCollectionFrag.appendChild(liRoot);
//         for (let handler of DoppelCore.toolHandler) {
//             await handler(meshCollectionFrag, doppelId)
//         }
//     }

//     DoppelCore.systemHandler.forEach((handler) => handler(meshCollectionFrag));

//     meshCollection.innerText = '';
//     meshCollection.appendChild(meshCollectionFrag);

//     // explicitly initialize tooltip
//     var tooltip_elems = document.querySelectorAll('.tooltipped');
//     var tooltip_instances = M.Tooltip.init(tooltip_elems, {});
// }


// async function updateMeshFromJsonObject(json) {
//     // [ 
//     //   {
//     //      "doppelId": (int) unique id for this mesh
//     //      "remove": (bool) is remove == true, simply remove the mesh
//     //      "meshName": (string) name for this mesh
//     //      "F": (array of int) vertex indices into V
//     //      "V": (array of float) vertex positions 
//     //      "VC": (array of float) vertex color
//     //      "TC": (array of float) texture coordinate for each vertex
//     //      "visibility": (bool) whether this mesh is visible
//     //      "textureArray": (array of short, optional) texture data
//     //      "width": (int, optional) texture width
//     //      "height": (int, optional) texture height
//     //   },
//     //   ...
//     // ]

//     for await (var [id, meshJson] of Object.entries(json)) {
//         var doppelId = meshJson["doppelId"];
//         if (meshJson["remove"]) {
//             var meshToBeRemoved = undefined;
//             canvas.meshGroup.traverse(function (obj) {
//                 if (obj instanceof THREE.Mesh && obj) {
//                     if (obj.doppelId == doppelId) {
//                         meshToBeRemoved = obj;
//                     }
//                 }
//             });
//             if (meshToBeRemoved) {
//                 canvas.meshGroup.remove(meshToBeRemoved);
//             }

//             var backFaceMesh = undefined;
//             canvas.backMeshGroup.traverse(function (obj) {
//                 if (obj instanceof THREE.Mesh && obj) {
//                     if (obj.doppelId == doppelId) {
//                         backFaceMesh = obj;
//                     }
//                 }
//             });
//             if (backFaceMesh) {
//                 canvas.backMeshGroup.remove(backFaceMesh);
//             }
//             await updateToolElement(meshToBeRemoved, meshJson["remove"]);
//         }
//         else {
//             var oldMesh = null;
//             canvas.meshGroup.traverse(function (obj) {
//                 if (obj instanceof THREE.Mesh && obj) {
//                     if (obj.doppelId == doppelId) {
//                         oldMesh = obj;
//                     }
//                 }
//             });

//             var geometry;
//             var material;

//             if (oldMesh == null) {
//                 geometry = new THREE.BufferGeometry();
//                 material = new THREE.MeshPhongMaterial({ color: 0xFFFFFF, flatShading: true, vertexColors: THREE.NoColors });
//             }
//             else {
//                 geometry = oldMesh.geometry;
//                 material = oldMesh.material;
//             }

//             // IMPORTANT
//             // If the mesh has face color, server automatically convert
//             // face color to vertex color (by duplicating the vertices)
//             function b64ToUint6(nChr) {
//                 return nChr > 64 && nChr < 91 ?
//                     nChr - 65
//                     : nChr > 96 && nChr < 123 ?
//                         nChr - 71
//                         : nChr > 47 && nChr < 58 ?
//                             nChr + 4
//                             : nChr === 43 ?
//                                 62
//                                 : nChr === 47 ?
//                                     63
//                                     :
//                                     0;
//             }
//             function base64DecToArr(sBase64, nBlockSize) {
//                 var sB64Enc = sBase64.replace(/[^A-Za-z0-9\+\/]/g, "");
//                 var nInLen = sB64Enc.length;
//                 var nOutLen = nBlockSize ? Math.ceil((nInLen * 3 + 1 >>> 2) / nBlockSize) * nBlockSize : nInLen * 3 + 1 >>> 2;
//                 var aBytes = new Uint8Array(nOutLen);

//                 for (var nMod3, nMod4, nUint24 = 0, nOutIdx = 0, nInIdx = 0; nInIdx < nInLen; nInIdx++) {
//                     nMod4 = nInIdx & 3;
//                     nUint24 |= b64ToUint6(sB64Enc.charCodeAt(nInIdx)) << 18 - 6 * nMod4;
//                     if (nMod4 === 3 || nInLen - nInIdx === 1) {
//                         for (nMod3 = 0; nMod3 < 3 && nOutIdx < nOutLen; nMod3++, nOutIdx++) {
//                             aBytes[nOutIdx] = nUint24 >>> (16 >>> nMod3 & 24) & 255;
//                         }
//                         nUint24 = 0;
//                     }
//                 }
//                 return aBytes;
//             }

//             if ("V" in meshJson && meshJson["V"].length > 0) {
//                 // 32 bit (4 byte) float
//                 var array = base64DecToArr(meshJson["V"], 4);
//                 geometry.setAttribute('position', new THREE.Float32BufferAttribute(new Float32Array(array.buffer), 3));
//                 geometry.deleteAttribute('normal');
//                 geometry.getAttribute('position').needsUpdate = true;
//             }
//             if ("F" in meshJson && meshJson["F"].length > 0) {
//                 // 32 bit (4 byte) int
//                 var array = base64DecToArr(meshJson["F"], 4);
//                 geometry.setIndex(new THREE.Uint32BufferAttribute(new Int32Array(array.buffer), 1));
//             }
//             if ("VC" in meshJson) {
//                 // 32 bit (4 byte) float
//                 // we accept VC even if VC.rows() != V.rows() to support erasing VC with undo/redo
//                 var array = base64DecToArr(meshJson["VC"], 4);
//                 geometry.setAttribute('color', new THREE.Float32BufferAttribute(new Float32Array(array.buffer), 3));
//                 geometry.getAttribute('color').needsUpdate = true;
//             }
//             if ("TC" in meshJson && meshJson["TC"].length > 0) {
//                 // 32 bit (4 byte) float
//                 var array = base64DecToArr(meshJson["TC"], 4);
//                 geometry.setAttribute('uv', new THREE.Float32BufferAttribute(new Float32Array(array.buffer), 2));
//                 geometry.getAttribute('uv').needsUpdate = true;
//             }
//             if ("textureArray" in meshJson && meshJson["textureArray"].length > 0) {
//                 // 8 bit (1 byte) uint
//                 var array = base64DecToArr(meshJson["textureArray"], 1);
//                 material.map = new THREE.DataTexture(new Uint8Array(array.buffer), meshJson["width"], meshJson["height"], THREE.RGBAFormat);
//                 material.map.needsUpdate = true;
//             }

//             if (material.map && geometry.hasAttribute('position') && geometry.hasAttribute('uv') && geometry.getAttribute('position').count == geometry.getAttribute('uv').count) {
//                 // texture
//                 material.vertexColors = THREE.NoColors;
//             } else if (geometry.hasAttribute('position') &&
//                 geometry.hasAttribute('color') &&
//                 geometry.getAttribute('position').count == geometry.getAttribute('color').count) {
//                 // vertex color (user-defined)
//                 material.vertexColors = THREE.VertexColors;
//             } else {
//                 // vertex color (default, grey)
//                 var buffer = new ArrayBuffer(geometry.getAttribute('position').count * 3 * 4);
//                 var view = new Float32Array(buffer);
//                 for (var i = 0; i < geometry.getAttribute('position').count; ++i) {
//                     for (var rgb = 0; rgb < 3; ++rgb) {
//                         view[i * 3 + rgb] = canvas.defaultColor[rgb];
//                     }
//                 }
//                 geometry.setAttribute('color', new THREE.Float32BufferAttribute(view, 3));
//                 geometry.getAttribute('color').needsUpdate = true;
//                 material.vertexColors = THREE.VertexColors;
//             }

//             geometry.computeVertexNormals();
//             material.needsUpdate = true;

//             var mesh;
//             if (oldMesh == null) {
//                 mesh = new THREE.Mesh(geometry, material);
//                 mesh.doppelId = doppelId;
//                 canvas.meshGroup.add(mesh);
//             } else {
//                 mesh = oldMesh;
//             }

//             if ("visibility" in meshJson) {
//                 mesh.visible = meshJson["visibility"];
//             }
//             if ("meshName" in meshJson) {
//                 mesh.name = meshJson["meshName"];
//             }

//             // create mesh for backface
//             {
//                 var backFaceMesh = undefined;
//                 canvas.backMeshGroup.traverse(function (obj) {
//                     if (obj instanceof THREE.Mesh && obj) {
//                         if (obj.doppelId == doppelId) {
//                             backFaceMesh = obj;
//                         }
//                     }
//                 });
//                 if (backFaceMesh) {
//                     backFaceMesh.geometry.copy(mesh.geometry);
//                 } else {
//                     var backFaceMaterial = new THREE.MeshPhongMaterial({ color: 0xf56c0a, flatShading: true, vertexColors: THREE.NoColors, side: THREE.BackSide });
//                     // when we use geometry (without clone()), threejs cause some error...
//                     // I understand this consumes large memory...
//                     backFaceMesh = new THREE.Mesh(geometry.clone(), backFaceMaterial);
//                     backFaceMesh.doppelId = doppelId;
//                     canvas.backMeshGroup.add(backFaceMesh);
//                 }

//                 if ("visibility" in meshJson) {
//                     backFaceMesh.visible = meshJson["visibility"];
//                 }
//             }

//             await updateToolElement(mesh, meshJson["remove"]);
//         }
//     }
// }

// async function syncParams(j) {
//     if ("timestamp" in j && j["timestamp"] > DoppelCore.strokeTimeStamp) {
//         // camera parameter
//         if ("target" in j) {
//             canvas.controls.target.set(j["target"].x, j["target"].y, j["target"].z);
//         }
//         if ("pos" in j) {
//             canvas.camera.position.set(j["pos"].x, j["pos"].y, j["pos"].z);
//             canvas.lastCameraPos = canvas.camera.position.clone();
//         }
//         if ("up" in j) {
//             canvas.camera.up.set(j["up"].x, j["up"].y, j["up"].z);
//             canvas.lastCameraUp = canvas.camera.up.clone();
//         }
//         if ("zoom" in j) {
//             canvas.camera.zoom = j["zoom"];
//             canvas.lastCameraZoom = canvas.camera.zoom;
//         }
//         canvas.camera.updateProjectionMatrix();
//         canvas.camera.lookAt(canvas.controls.target.clone());
//         canvas.camera.updateMatrixWorld(true);
//     }
// }



// export async function syncMeshes(j) {
//     var firstMesh = (canvas.meshGroup.children.length == 0);
//     if ("meshes" in j) {
//         await updateMeshFromJsonObject(j["meshes"]);
//         if (firstMesh) {
//             fitToFrame();
//         }
//         resetCamera(true);
//     }
// }

