import { request } from '../../js/request.js';
import { constructMeshLiFromJson } from '../../js/constructMeshLiFrom.js';

const text = {
    "Toggle visibility": { "en": "Toggle visibility", "ja": "表示/非表示" }
};

////
// UI
const generateUI = async function () {
    constructMeshLiFromJson.handlers.push(
        function (json, liRoot) {
            // for element, we cannot use getElementById ...
            const iIcon = liRoot.querySelector("#icon_" + json["UUID"]);
            const parameters = {};
            parameters["meshes"] = [json["UUID"]];
            return request("meshErrorInfo", parameters).then((response) => {
                const responseJson = JSON.parse(response);
                const withoutError = responseJson["meshes"][json["UUID"]]["closed"] && responseJson["meshes"][json["UUID"]]["edgeManifold"] && responseJson["meshes"][json["UUID"]]["vertexManifold"];
                iIcon.setAttribute("class", (withoutError ? "material-icons teal-text text-lighten-2 circle" : "material-icons orange-text text-lighten-2 circle"));
                iIcon.innerText = (withoutError ? "check_circle" : "warning");
            });
        }
    );
}

export const init = async function () {
    await generateUI();
}

