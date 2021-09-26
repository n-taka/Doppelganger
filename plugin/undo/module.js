import { WSTasks } from '../../js/WSTasks.js';
import { UI } from '../../js/UI.js';
import { Canvas } from '../../js/Canvas.js';
import { getText } from '../../js/Text.js';
import { request } from '../../js/request.js';
import { constructMeshFromJson } from '../../js/constructMeshFromJson.js';

const text = {
    "Undo": { "en": "Undo", "ja": "Undo" }
};

////
// UI
const generateUI = async function () {
    ////
    // button
    {
        const li = document.createElement("li");
        {
            const a = document.createElement("a");
            a.addEventListener('click', function () {
                request("undo");
            });
            a.setAttribute("class", "tooltipped");
            a.setAttribute("data-position", "bottom");
            a.setAttribute("data-tooltip", getText(text, "Undo"));
            {
                const i = document.createElement("i");
                i.innerHTML = "undo";
                i.setAttribute("class", "material-icons");
                a.appendChild(i);
            }
            li.appendChild(a);
        }
        UI.topMenuRightUl.appendChild(li);
    }
}

////
// WS API
const undo = async function (parameters) {
    if (!Canvas.UUIDToMesh) {
        Canvas.UUIDToMesh = {};
    }
    const isFirstMesh = (Canvas.meshGroup.children.length == 0);
    if ("meshes" in parameters) {
        for (let meshUUID in parameters["meshes"]) {
            // erase old mesh (not optimal...)
            if (meshUUID in Canvas.UUIDToMesh) {
                const mesh = Canvas.UUIDToMesh[meshUUID];
                // remove from scene
                Canvas.meshGroup.remove(mesh);
                // remove from map
                delete Canvas.UUIDToMesh[meshUUID];
                // dispose geometry/material
                //   children[0]: backFaceMesh, geometry is shared
                mesh.children[0].material.dispose();
                mesh.geometry.dispose();
                mesh.material.dispose();
            }
            if (!("remove" in parameters["meshes"][meshUUID] && parameters["meshes"][meshUUID]["remove"])) {
                const updatedMesh = constructMeshFromJson(parameters["meshes"][meshUUID]);
                Canvas.meshGroup.add(updatedMesh);
                Canvas.UUIDToMesh[meshUUID] = updatedMesh;
            }
        }
        if (isFirstMesh) {
            Canvas.fitToFrame();
        }
        Canvas.resetCamera(true);
    }
}

export const init = async function () {
    await generateUI();
    WSTasks["undo"] = undo;
}

