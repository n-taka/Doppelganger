import { WSTasks } from '../../js/WSTasks.js';
import { UI } from '../../js/UI.js';
import { Canvas } from '../../js/Canvas.js';
import { getText } from '../../js/Text.js';
import { request } from '../../js/request.js';
import { constructMeshFromJson } from '../../js/constructMeshFromJson.js';

const text = {
    "Polygon": { "en": "Polygon", "ja": "ポリゴン" },
    "Import polygon": { "en": "Import polygon", "ja": "ポリゴン読み込み" }
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
                loadMeshes('.obj,.stl,.ply,.wrl');
            });
            a.setAttribute("class", "tooltipped");
            a.setAttribute("data-position", "bottom");
            a.setAttribute("data-tooltip", getText(text, "Import polygon"));
            a.innerHTML = getText(text, "Polygon");

            {
                const i = document.createElement("i");
                i.innerHTML = "add";
                i.setAttribute("class", "material-icons left");
                a.appendChild(i);
            }
            li.appendChild(a);
        }
        UI.topMenuLeftUl.appendChild(li);
    }
}

const loadMeshes = async function (formatToBeAccepted) {
    // http://pirosikick.hateblo.jp/entry/2014/08/11/003235
    if (loadMeshes.inputElement) {
        document.body.removeChild(loadMeshes.inputElement);
    }

    const inputElement = loadMeshes.inputElement = document.createElement('input');

    inputElement.type = 'file';
    inputElement.multiple = 'multiple';
    inputElement.accept = formatToBeAccepted;

    inputElement.style.visibility = 'hidden';
    inputElement.style.position = 'absolute';
    inputElement.style.left = '-9999px';

    document.body.appendChild(inputElement);

    // https://www.html5rocks.com/ja/tutorials/file/dndfiles/
    // https://threejs.org/docs/#examples/loaders/GLTFLoader
    // https://qiita.com/weal/items/1a2af81138cd8f49937d
    inputElement.addEventListener('change', function (e) {
        if (e.target.files.length > 0) {
            for (let file of e.target.files) {
                loadMesh(file);
            }
        }
    }, false);
    inputElement.click();
}

const loadMesh = async function (file) {
    const freader = new FileReader();
    freader.onload = function (event) {
        const fileName = file.name;
        const type = fileName.split('.');
        const fileId = Math.random().toString(36).substring(2, 9);
        const base64 = freader.result.substring(freader.result.indexOf(',') + 1);
        // split into 0.5MB packets. Due to the default limit (up to 1MB) of boost beast.
        const packetSize = 500000;
        const packetCount = Math.ceil(base64.length / packetSize);

        for (let packet = 0; packet < packetCount; ++packet) {
            const base64Packet = base64.substring(packet * packetSize, (packet + 1) * packetSize);
            const json = {
                "mesh": {
                    "name": type.slice(0, -1).join("."),
                    "file": {
                        "id": fileId,
                        "size": base64.length,
                        "packetId": packet,
                        "packetSize": packetSize,
                        "packetTotal": packetCount,
                        "type": type[type.length - 1].toLowerCase(),
                        "base64Packet": base64Packet
                    }
                }
            };
            request("loadPolygonMesh", json);
        }
    };
    freader.readAsDataURL(file);
}

////
// WS API
const loadPolygonMesh = async function (parameters) {
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
    WSTasks["loadPolygonMesh"] = loadPolygonMesh;
}

