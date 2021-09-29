import { UI } from '../../js/UI.js';
import { WSTasks } from '../../js/WSTasks.js';
import { getText } from '../../js/Text.js';
import { request } from '../../js/request.js';
import { Canvas } from '../../js/Canvas.js';
import { constructMeshFromParameters } from '../../js/constructMeshFrom.js';
import { constructMeshLiFromParameters } from '../../js/constructMeshLiFrom.js';

const text = {
    "Remove All": { "en": "Remove All", "ja": "すべて削除" }
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
                const parameters = {};
                parameters["meshes"] = Object.keys(Canvas.UUIDToMesh);
                request("removeAllMeshes", parameters);
            });
            a.setAttribute("class", "tooltipped");
            a.setAttribute("data-position", "top");
            a.setAttribute("data-tooltip", getText(text, "Remove All"));
            {
                const i = document.createElement("i");
                i.innerText = "delete_forever";
                i.setAttribute("class", "material-icons");
                a.appendChild(i);
            }
            li.appendChild(a);
        }
        UI.bottomMenuLeftUl.appendChild(li);
    }
}

////
// WS API
const removeAllMeshes = async function (parameters) {
    await constructMeshFromParameters(parameters);
    await constructMeshLiFromParameters(parameters);
}

export const init = async function () {
    await generateUI();
    WSTasks["removeAllMeshes"] = removeAllMeshes;
}

