import { WSTasks } from '../../js/WSTasks.js';
import { UI } from '../../js/UI.js';
import { getText } from '../../js/Text.js';
import { request } from '../../js/request.js';
import { constructMeshFromParameters } from '../../js/constructMeshFrom.js';
import { constructMeshLiFromParameters } from '../../js/constructMeshLiFrom.js';

const text = {
    "CAD": { "en": "CAD", "ja": "CAD" },
    "Import CAD": { "en": "Import CAD", "ja": "CAD読み込み" }
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
                loadCADData('.iges,.igs,.step,.stp');
            });
            a.setAttribute("class", "tooltipped");
            a.setAttribute("data-position", "bottom");
            a.setAttribute("data-tooltip", getText(text, "Import CAD"));
            a.innerText = getText(text, "CAD");

            {
                const i = document.createElement("i");
                i.innerText = "add";
                i.setAttribute("class", "material-icons left");
                a.appendChild(i);
            }
            li.appendChild(a);
        }
        UI.topMenuLeftUl.appendChild(li);
    }
}

const loadCADData = async function (formatToBeAccepted) {
    // http://pirosikick.hateblo.jp/entry/2014/08/11/003235
    if (loadCADData.inputElement) {
        document.body.removeChild(loadCADData.inputElement);
    }

    const inputElement = loadCADData.inputElement = document.createElement('input');

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
                loadSingleCADData(file);
            }
        }
    }, false);
    inputElement.click();
}

const loadSingleCADData = async function (file) {
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
                "CAD": {
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
            request("loadCADData", json);
        }
    };
    freader.readAsDataURL(file);
}

////
// WS API
const handler = async function (parameters) {
    await constructMeshFromParameters(parameters);
    await constructMeshLiFromParameters(parameters);
}

export const init = async function () {
    await generateUI();
    WSTasks["loadCADData"] = handler;
}

