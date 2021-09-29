import { WSTasks } from '../../js/WSTasks.js';
import { UI } from '../../js/UI.js';
import { getText } from '../../js/Text.js';
import { request } from '../../js/request.js';
import { constructMeshFromParameters } from '../../js/constructMeshFrom.js';
import { constructMeshLiFromParameters } from '../../js/constructMeshLiFrom.js';

const text = {
    "Redo": { "en": "Redo", "ja": "Redo" }
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
                request("redo");
            });
            a.setAttribute("class", "tooltipped");
            a.setAttribute("data-position", "bottom");
            a.setAttribute("data-tooltip", getText(text, "Redo"));
            {
                const i = document.createElement("i");
                i.innerText = "redo";
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
const redo = async function (parameters) {
    await constructMeshFromParameters(parameters);
    await constructMeshLiFromParameters(parameters);
}

export const init = async function () {
    await generateUI();

    WSTasks["redo"] = redo;

    // keyboard event
    document.addEventListener("keydown", (function (e) {
        var keycode = e.code;
        if (keycode == 'KeyZ') {
            // 'z' key
            if (((e.ctrlKey && !e.metaKey) || (!e.ctrlKey && e.metaKey)) && e.shiftKey) {
                // 'Ctrl-Shift-z'
                request("redo");
            }
        }
    }));
}

