import { UI } from '../../../js/UI.js';
import { APICall } from '../../../js/APICall.js';

var text = {
    "tooltip": { "default": "Plugins", "ja": "プラグイン" }
};

const generateUI = function () {
    ////
    // modal
    const modal = document.createElement("div");
    {
        modal.setAttribute("class", "modal");
        // content
        {
            const modalContentDiv = document.createElement('div');
            modalContentDiv.setAttribute("class", "modal-content");
            const plugins = JSON.parse(await APICall("listPlugins", {}));
            // todo: update innerHTML based on value
            //   see: https://materializecss.com/table.html
            //   see: https://materializecss.com/dropdown.html#!
            console.log(plugins);
            modalContentDiv.innerHTML = `
                <h4>Oops!</h4>
                <p>
                    Some error is happening. Please debug me!
                </p>`;
            const installedPlugins = JSON.parse(await APICall("getInstalledPlugins", {}));
            // todo: update innerHTML based on value
            modal.appendChild(modalContentDiv);
        }
        // footer
        {
            const modalFooterDiv = document.createElement("div");
            modalFooterDiv.setAttribute("class", "modal-footer");
            {
                const modalFooterCloseA = document.createElement("a");
                modalFooterCloseA.setAttribute("class", "modal-close waves-effect waves-green btn-flat");
                modalFooterCloseA.setAttribute("href", "#!");
                modalFooterCloseA.innerHTML = "Close";
                modalFooterDiv.appendChild(modalFooterCloseA);
            }
            {
                // todo add "apply" button to change the version of the plugin
                // after hitting "apply" button, we need to (reload the window or Plugin.init())
            }
            modal.appendChild(modalFooterDiv);
        }
        UI.modalDiv.appendChild(modal);
    }
    ////
    // button
    {
        const li = document.createElement("li");
        {
            const a = document.createElement("a");
            a.addEventListener('click', function () {
                const instance = M.Modal.getInstance(modal);
                instance.open();
            });
            a.setAttribute("class", "tooltipped");
            a.setAttribute("data-position", "top");
            a.setAttribute("data-tooltip", text["tooltip"][DoppelCore.language in text["tooltip"] ? DoppelCore.language : "default"]);
            {
                const i = document.createElement("i");
                a.appendChild(i);
                i.innerHTML = "playlist_add";
                i.setAttribute("class", "material-icons");
            }
            li.appendChild(a);
        }
        UI.bottomMenuUl.appendChild(li);
    }
};

////
// UI
export const init = function () {
    generateUI();

    const modal_elems = document.querySelectorAll('.modal');
    const modal_instances = M.Modal.init(modal_elems, {});
}
