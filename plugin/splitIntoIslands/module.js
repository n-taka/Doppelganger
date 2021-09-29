import { WSTasks } from '../../js/WSTasks.js';
import { getText } from '../../js/Text.js';
import { request } from '../../js/request.js';
import { constructMeshFromParameters } from '../../js/constructMeshFrom.js';
import { constructMeshLiFromParameters, constructMeshLiFromJson } from '../../js/constructMeshLiFrom.js';

const text = {
    "Split into islands": { "default": "Split into islands", "ja": "アイランドに分割" }
};

////
// UI
const generateUI = async function () {
    constructMeshLiFromJson.handlers.push(
        function (json, liRoot) {
            // for element, we cannot use getElementById ...
            const pButtons = liRoot.querySelector("#buttons_" + json["UUID"]);
            {
                const a = document.createElement("a");
                a.setAttribute("class", "tooltipped");
                a.setAttribute("data-position", "top");
                a.setAttribute("data-tooltip", getText(text, "Split into islands"));
                a.addEventListener('click', function (e) {
                    const parameters = {};
                    parameters["meshes"] = [json["UUID"]];
                    request("splitIntoIslands", parameters);
                    // don't fire click event on the parent (e.g. outlineOnClick)
                    e.stopPropagation();
                });
                const instance = M.Tooltip.init(a, {});
                
                {
                    const i = document.createElement("i");
                    i.setAttribute("class", "material-icons teal-text text-lighten-2");
                    i.innerText = "widgets";
                    a.appendChild(i);
                }
                pButtons.appendChild(a);
            }
        }
    );
}

////
// WS API
const splitIntoIslands = async function (parameters) {
    await constructMeshFromParameters(parameters);
    await constructMeshLiFromParameters(parameters);
}

export const init = async function () {
    await generateUI();
    WSTasks["splitIntoIslands"] = splitIntoIslands;
}

