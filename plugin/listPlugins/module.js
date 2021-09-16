import { Core } from '../../js/Core.js';
import { UI } from '../../js/UI.js';
import { request } from '../../js/request.js';

var text = {
    "tooltip": { "default": "Plugins", "ja": "プラグイン" }
};

const generateUI = async function () {
    ////
    // modal
    const modal = document.createElement("div");
    {
        modal.setAttribute("class", "modal");
        // content
        {
            const modalContentDiv = document.createElement('div');
            modalContentDiv.setAttribute("class", "modal-content");
            const plugins = JSON.parse(await request("listPlugins", {}));
            // todo: update innerHTML based on value
            //   see: https://materializecss.com/table.html
            //   see: https://materializecss.com/dropdown.html#!
            console.log(plugins);
            modalContentDiv.innerHTML = `
                <h4>Oops!</h4>
                <p>
                    Some error is happening. Please debug me!
                </p>`;
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
                // todo
                //   changing the version of plugins need rebooting.
                //   so, we prompt to reboot this system.
                //     for better usage, we perform loading/unloading dll on the fly...
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
            a.setAttribute("data-tooltip", text["tooltip"][Core.language in text["tooltip"] ? Core.language : "default"]);
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
export const init = async function () {
    console.log("listPlugins.init()");
    await generateUI();
}
