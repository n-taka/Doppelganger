import { Core } from '../../js/Core.js';
import { UI } from '../../js/UI.js';
import { Plugin } from '../../js/Plugin.js';
import { getText } from '../../js/Text.js';
import { request } from "../../js/request.js";

const text = {
    "Plugin Manager": { "en": "Plugin Manager", "ja": "プラグインマネージャ" },
    "Plugin": { "en": "Plugin", "ja": "プラグイン" },
    "Apply & Shutdown": { "en": "Apply & Shutdown", "ja": "更新して終了" },
    "Cancel": { "en": "Cancel", "ja": "キャンセル" },
    "Name": { "en": "Name", "ja": "プラグイン名" },
    "Current": { "en": "Current", "ja": "現在のバージョン" },
    "Change to...": { "en": "Change to...", "ja": "アップデート先" },
    "Description": { "en": "Description", "ja": "説明" },
    "Uninstall": { "en": "Uninstall", "ja": "アンインストール" },
    "Don't install": { "en": "Don't install", "ja": "インストールしない" }
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
                heading.innerText = getText(text, "Plugin Manager");
                modalContentDiv.appendChild(heading);
            }
            {
                const table = document.createElement("table");
                table.setAttribute("class", "striped");
                table.setAttribute("style", "table-layout: fixed;");
                {
                    const thead = document.createElement("thead");
                    {
                        const tr = document.createElement("tr");
                        {
                            const thName = document.createElement("th");
                            thName.setAttribute("style", "width: 15%;");
                            {
                                const spanName = document.createElement("span");
                                spanName.setAttribute("class", "truncate");
                                spanName.innerText = getText(text, "Name");
                                thName.appendChild(spanName);
                            }
                            tr.appendChild(thName);
                        }
                        {
                            const thCurrent = document.createElement("th");
                            thCurrent.setAttribute("style", "width: 15%;");
                            {
                                const spanCurrent = document.createElement("span");
                                spanCurrent.setAttribute("class", "truncate");
                                spanCurrent.innerText = getText(text, "Current");
                                thCurrent.appendChild(spanCurrent);
                            }
                            tr.appendChild(thCurrent);
                        }
                        {
                            const thAction = document.createElement("th");
                            thAction.setAttribute("style", "width: 20%;");
                            {
                                const spanAction = document.createElement("span");
                                spanAction.setAttribute("class", "truncate");
                                spanAction.innerText = getText(text, "Change to...");
                                thAction.appendChild(spanAction);
                            }
                            tr.appendChild(thAction);
                        }
                        {
                            const thDescription = document.createElement("th");
                            thDescription.setAttribute("style", "width: 50%;");
                            {
                                const spanDescription = document.createElement("span");
                                spanDescription.setAttribute("class", "truncate");
                                spanDescription.innerText = getText(text, "Description");
                                thDescription.appendChild(spanDescription);
                            }
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
                                {
                                    const spanName = document.createElement("span");
                                    spanName.setAttribute("class", "truncate");
                                    spanName.innerText = name;
                                    tdName.appendChild(spanName);
                                }
                                tr.appendChild(tdName);
                            }
                            {
                                const tdCurrent = document.createElement("td");
                                {
                                    const spanCurrent = document.createElement("span");
                                    spanCurrent.setAttribute("class", "truncate");
                                    spanCurrent.innerText = plugin["installedVersion"];
                                    tdCurrent.appendChild(spanCurrent);
                                }
                                tr.appendChild(tdCurrent);
                            }
                            {
                                const tdVersion = document.createElement("td");
                                // text of the dropdownA need to be accessible from dropdown elements
                                const dropdownSpan = document.createElement("span");
                                {
                                    const dropdownA = document.createElement("a");
                                    dropdownA.setAttribute("class", "dropdown-trigger btn-flat");
                                    dropdownA.setAttribute("style", "width: 100%; text-transform: none; text-overflow: ellipsis;");
                                    dropdownA.setAttribute("href", "#");
                                    dropdownA.setAttribute("data-target", "versionDropdown" + name);
                                    {
                                        const dropdownI = document.createElement("i");
                                        dropdownI.setAttribute("class", "material-icons right");
                                        dropdownI.innerText = "arrow_drop_down";
                                        dropdownA.appendChild(dropdownI);
                                    }
                                    {
                                        dropdownSpan.setAttribute("class", "truncate");
                                        dropdownSpan.setAttribute("style", "width: calc(100% - 40px);");
                                        dropdownSpan.innerText = (plugin["installedVersion"].length > 0) ? plugin["installedVersion"] : getText(text, "Don't install");
                                        dropdownA.appendChild(dropdownSpan);
                                    }
                                    tdVersion.appendChild(dropdownA);
                                }
                                {
                                    const dropdownUl = document.createElement("ul");
                                    dropdownUl.setAttribute("id", "versionDropdown" + name);
                                    dropdownUl.setAttribute("class", "dropdown-content");
                                    // we add special entry "uninstall" or "Don't install" if we already installed this plugin
                                    if (plugin["optional"]) {
                                        plugin["versions"].push((plugin["installedVersion"].length > 0) ? "Uninstall" : "Don't install");
                                    }
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
                                            });
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
                                if (Core.language in plugin["description"]) {
                                    tdDescription.innerText = plugin["description"][Core.language];
                                } else {
                                    tdDescription.innerText = plugin["description"]["en"];
                                }
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
                modalFooterApplyA.addEventListener("click", async function () {
                    await request("updatePlugins", update);
                    await request("shutdown", {});
                });
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
            a.setAttribute("data-tooltip", getText(text, "Plugin"));
            {
                const i = document.createElement("i");
                a.appendChild(i);
                i.innerHTML = "build";
                i.setAttribute("class", "material-icons");
            }
            li.appendChild(a);
        }
        UI.bottomMenuLeftUl.appendChild(li);
    }
};

////
// UI
export const init = async function () {
    await generateUI();
}
