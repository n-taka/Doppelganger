import { Core } from '../../js/Core.js';
import { UI } from '../../js/UI.js';
import { Plugin } from '../../js/Plugin.js';
import { getText } from '../../js/Text.js';

const text = {
    "tooltip": { "ja": "プラグイン" },
    "Apply & Shutdown": { "ja": "更新して終了" },
    "Cancel": { "ja": "キャンセル" },
    "Name": { "ja": "プラグイン名" },
    "Current": { "ja": "現在のバージョン" },
    "Change to": { "ja": "アップデート先" },
    "Description": { "ja": "説明" },
    "Uninstall": { "ja": "アンインストール" },
    "Don't install": { "ja": "インストールしない" }
};

const update = {};

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
            {
                const heading = document.createElement("h4");
                heading.innerText = "Plugin manager";
                modalContentDiv.appendChild(heading);
            }
            {
                const table = document.createElement("table");
                table.setAttribute("class", "striped");
                {
                    const thead = document.createElement("thead");
                    {
                        const tr = document.createElement("tr");
                        {
                            const thName = document.createElement("th");
                            thName.setAttribute("style", "width: 15%;");
                            thName.innerText = getText(text, "Name");
                            tr.appendChild(thName);
                        }
                        {
                            const thCurrent = document.createElement("th");
                            thCurrent.setAttribute("style", "width: 15%;");
                            thCurrent.innerText = getText(text, "Current");
                            tr.appendChild(thCurrent);
                        }
                        {
                            const thAction = document.createElement("th");
                            thAction.setAttribute("style", "width: 15%;");
                            thAction.innerText = getText(text, "Change to");
                            tr.appendChild(thAction);
                        }
                        {
                            const thDescription = document.createElement("th");
                            thDescription.setAttribute("style", "width: 55%;");
                            thDescription.innerText = getText(text, "Description");
                            tr.appendChild(thDescription);
                        }
                        thead.appendChild(tr);
                    }
                    table.appendChild(thead);
                }
                {
                    const tbody = document.createElement("tbody");
                    {
                        // see: https://materializecss.com/table.html
                        // see: https://materializecss.com/dropdown.html#!
                        const pluginList = JSON.parse(JSON.stringify(Plugin.pluginList));
                        for (let name in pluginList) {
                            const plugin = pluginList[name];
                            const tr = document.createElement("tr");
                            {
                                const tdName = document.createElement("td");
                                tdName.innerText = name;
                                tr.appendChild(tdName);
                            }
                            {
                                const tdCurrent = document.createElement("td");
                                tdCurrent.innerText = plugin["installedVersion"];
                                tr.appendChild(tdCurrent);
                            }
                            {
                                const tdVersion = document.createElement("td");
                                // text of the dropdownA need to be accessible from dropdown elements
                                const dropdownSpan = document.createElement("span");
                                {
                                    const dropdownA = document.createElement("a");
                                    dropdownA.setAttribute("class", "dropdown-trigger btn-flat");
                                    dropdownA.setAttribute("style", "width: 100%; text-transform: none;");
                                    dropdownA.setAttribute("href", "#");
                                    dropdownA.setAttribute("data-target", "versionDropdown" + name);
                                    {
                                        dropdownSpan.innerText = (plugin["installedVersion"].length > 0) ? plugin["installedVersion"] : getText(text, "Don't install");
                                        dropdownA.appendChild(dropdownSpan);
                                    }
                                    {
                                        const dropdownI = document.createElement("i");
                                        dropdownI.setAttribute("class", "material-icons right");
                                        dropdownI.innerText = "arrow_drop_down";
                                        dropdownA.appendChild(dropdownI);
                                    }
                                    tdVersion.appendChild(dropdownA);
                                }
                                {
                                    const dropdownUl = document.createElement("ul");
                                    dropdownUl.setAttribute("id", "versionDropdown" + name);
                                    dropdownUl.setAttribute("class", "dropdown-content");
                                    // we add special entry "uninstall" or "Don't install" if we already installed this plugin
                                    plugin["versions"].push((plugin["installedVersion"].length > 0) ? "Uninstall" : "Don't install");
                                    for (let version of plugin["versions"]) {
                                        const versionLi = document.createElement("li");
                                        {
                                            const versionA = document.createElement("a");
                                            versionA.setAttribute("href", "#!");
                                            versionA.innerText = (version == "Uninstall" || version == "Don't install") ? getText(text, version) : version;
                                            versionA.addEventListener("click", function () {
                                                dropdownSpan.innerText = (version == "Uninstall" || version == "Don't install") ? getText(text, version) : version;
                                                delete update[name];
                                                if (plugin["installedVersion"] != version && version != "Don't install") {
                                                    update[name] = version;
                                                }
                                                console.log(update);
                                            });
                                            // todo addEventListener...
                                            versionLi.appendChild(versionA);
                                        }
                                        dropdownUl.appendChild(versionLi);
                                    }
                                    tdVersion.appendChild(dropdownUl);
                                }
                                tr.appendChild(tdVersion);
                            }
                            {
                                const tdDescription = document.createElement("td");
                                tdDescription.innerText = plugin["description"];
                                tr.appendChild(tdDescription);
                            }
                            tbody.appendChild(tr);
                        }
                    }
                    table.appendChild(tbody);
                }
                modalContentDiv.appendChild(table);
            }
            modal.appendChild(modalContentDiv);
        }
        // footer
        {
            const modalFooterDiv = document.createElement("div");
            modalFooterDiv.setAttribute("class", "modal-footer");
            {
                const modalFooterApplyA = document.createElement("a");
                modalFooterApplyA.setAttribute("class", "modal-close waves-effect waves-green btn-flat");
                modalFooterApplyA.setAttribute("href", "#!");
                modalFooterApplyA.innerHTML = getText(text, "Apply & Shutdown");
                // todo addEventListener ...
                //   send this.update to server and update plugins (installed.json), then reboot.
                modalFooterDiv.appendChild(modalFooterApplyA);
            }
            {
                const modalFooterCancelA = document.createElement("a");
                modalFooterCancelA.setAttribute("class", "modal-close waves-effect waves-green btn-flat");
                modalFooterCancelA.setAttribute("href", "#!");
                modalFooterCancelA.innerHTML = getText(text, "Cancel");
                modalFooterDiv.appendChild(modalFooterCancelA);
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
            a.setAttribute("data-tooltip", getText(text, "tooltip"));
            {
                const i = document.createElement("i");
                a.appendChild(i);
                i.innerHTML = "build";
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
    await generateUI();
}
