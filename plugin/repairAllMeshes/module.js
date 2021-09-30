import { UI } from '../../js/UI.js';
import { WSTasks } from '../../js/WSTasks.js';
import { getText } from '../../js/Text.js';
import { request } from '../../js/request.js';
import { Canvas } from '../../js/Canvas.js';
import { constructMeshFromParameters } from '../../js/constructMeshFrom.js';
import { constructMeshLiFromParameters } from '../../js/constructMeshLiFrom.js';

const text = {
    "Repair all": { "en": "Repair all", "ja": "すべてエラー修正" }
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
                request("repairAllMeshes", parameters);
            });
            a.setAttribute("class", "tooltipped");
            a.setAttribute("data-position", "top");
            a.setAttribute("data-tooltip", getText(text, "Repair all"));
            {
                const i = document.createElement("i");
                i.innerText = "healing";
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
const repairAllMeshes = async function (parameters) {
    await constructMeshFromParameters(parameters);
    await constructMeshLiFromParameters(parameters);
}

export const init = async function () {
    await generateUI();
    WSTasks["repairAllMeshes"] = repairAllMeshes;
}

