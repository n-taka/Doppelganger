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

