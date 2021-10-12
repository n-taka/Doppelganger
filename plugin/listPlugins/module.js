import { Core } from '../../js/Core.js';
import { UI } from '../../js/UI.js';
import { Plugin } from '../../js/Plugin.js';
import { getText } from '../../js/Text.js';
import { request } from "../../js/request.js";

const text = {
    "Plugin Manager": { "en": "Plugin Manager", "ja": "プラグインマネージャ" },
    "Top, Left": { "en": "Top, Left", "ja": "トップメニュー (左)" },
    "Top, Right": { "en": "Top, Right", "ja": "トップメニュー (右)" },
    "Mesh": { "en": "Mesh", "ja": "メッシュ" },
    "Bottom, Info": { "en": "Bottom, Info", "ja": "ボトムメニュー (情報表示)" },
    "Bottom, Left": { "en": "Bottom, Left", "ja": "ボトムメニュー (左)" },
    "Bottom, Right": { "en": "Bottom, Right", "ja": "ボトムメニュー (右)" },
    "No UI": { "en": "No UI", "ja": "UI なし" },
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
                // create tables and append to modalContentDiv
                const tables = {};
                {
                    {
                        const heading = document.createElement("h5");
                        heading.innerText = getText(text, "Top, Left");
                        modalContentDiv.appendChild(heading);
                    }
                    tables["topLeft"] = document.createElement("table");
                    modalContentDiv.appendChild(tables["topLeft"]);
                    {
                        const heading = document.createElement("h5");
                        heading.innerText = getText(text, "Top, Right");
                        modalContentDiv.appendChild(heading);
                    }
                    tables["topRight"] = document.createElement("table");
                    modalContentDiv.appendChild(tables["topRight"]);
                    {
                        const heading = document.createElement("h5");
                        heading.innerText = getText(text, "Mesh");
                        modalContentDiv.appendChild(heading);
                    }
                    tables["mesh"] = document.createElement("table");
                    modalContentDiv.appendChild(tables["mesh"]);
                    {
                        const heading = document.createElement("h5");
                        heading.innerText = getText(text, "Bottom, Info");
                        modalContentDiv.appendChild(heading);
                    }
                    tables["bottomInfo"] = document.createElement("table");
                    modalContentDiv.appendChild(tables["bottomInfo"]);
                    {
                        const heading = document.createElement("h5");
                        heading.innerText = getText(text, "Bottom, Left");
                        modalContentDiv.appendChild(heading);
                    }
                    tables["bottomLeft"] = document.createElement("table");
                    modalContentDiv.appendChild(tables["bottomLeft"]);
                    {
                        const heading = document.createElement("h5");
                        heading.innerText = getText(text, "Bottom, Right");
                        modalContentDiv.appendChild(heading);
                    }
                    tables["bottomRight"] = document.createElement("table");
                    modalContentDiv.appendChild(tables["bottomRight"]);
                    {
                        const heading = document.createElement("h5");
                        heading.innerText = getText(text, "No UI");
                        modalContentDiv.appendChild(heading);
                    }
                    tables["none"] = document.createElement("table");
                    modalContentDiv.appendChild(tables["none"]);
                }

                // create thead and tbody
                for (let [key, table] of Object.entries(tables)) {
                    table.setAttribute("class", "striped");
                    table.setAttribute("style", "table-layout: fixed;");
                    {
                        const thead = document.createElement("thead");
                        thead.setAttribute("style", "user-select: none;");
                        {
                            const tr = document.createElement("tr");
                            {
                                const thName = document.createElement("th");
                                thName.setAttribute("style", "width: 15%;");
                                {
                                    const spanName = document.createElement("span");
                                    spanName.setAttribute("class", "center-align truncate");
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
                                    spanCurrent.setAttribute("class", "center-align truncate");
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
                                    spanAction.setAttribute("class", "center-align truncate");
                                    spanAction.innerText = getText(text, "Change to...");
                                    thAction.appendChild(spanAction);
                                }
                                tr.appendChild(thAction);
                            }
                            {
                                const thDescription = document.createElement("th");
                                thDescription.setAttribute("style", "width: 40%;");
                                {
                                    const spanDescription = document.createElement("span");
                                    spanDescription.setAttribute("class", "center-align truncate");
                                    spanDescription.innerText = getText(text, "Description");
                                    thDescription.appendChild(spanDescription);
                                }
                                tr.appendChild(thDescription);
                            }
                            {
                                const thBlank = document.createElement("th");
                                thBlank.setAttribute("style", "width: 5%;");
                                {
                                }
                                tr.appendChild(thBlank);
                            }
                            {
                                const thBlank = document.createElement("th");
                                thBlank.setAttribute("style", "width: 5%;");
                                {
                                }
                                tr.appendChild(thBlank);
                            }
                            thead.appendChild(tr);
                        }
                        table.appendChild(thead);

                        const tbody = document.createElement("tbody");
                        table.appendChild(tbody);
                    }
                }

                // see: https://materializecss.com/table.html
                //      https://materializecss.github.io/materialize/select.html
                // because we modify the content, we perform deep-copy here
                const pluginList = JSON.parse(JSON.stringify(Plugin.pluginList));
                for (let plugin of pluginList) {
                    const tr = document.createElement("tr");
                    {
                        const tdName = document.createElement("td");
                        {
                            const spanName = document.createElement("span");
                            spanName.setAttribute("class", "center-align truncate");
                            spanName.innerText = plugin["name"];
                            tdName.appendChild(spanName);
                        }
                        tr.appendChild(tdName);
                    }
                    {
                        const tdCurrent = document.createElement("td");
                        {
                            const spanCurrent = document.createElement("span");
                            spanCurrent.setAttribute("class", "center-align truncate");
                            spanCurrent.innerText = plugin["installedVersion"];
                            tdCurrent.appendChild(spanCurrent);
                        }
                        tr.appendChild(tdCurrent);
                    }
                    {
                        const tdVersion = document.createElement("td");

                        // we add special entry "uninstall" or "Don't install" if we already installed this plugin
                        if (plugin["optional"]) {
                            plugin["versions"].push("");
                        }
                        {
                            const select = document.createElement('select');
                            select.setAttribute("data-plugin-name", plugin["name"]);

                            for (let version of plugin["versions"]) {
                                const option = document.createElement('option');
                                option.setAttribute('value', version);
                                if ((plugin["installedVersion"].length > 0) ? (version == plugin["installedVersion"]) : (version == "")) {
                                    option.setAttribute('selected', "");
                                }
                                option.innerText = ((version == "") ? getText(text, ((plugin["installedVersion"].length > 0) ? "Uninstall" : "Don't install")) : version);
                                select.appendChild(option);
                            }
                            tdVersion.appendChild(select);
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
                    {
                        const tdUp = document.createElement("td");
                        {
                            const a = document.createElement("a");
                            a.addEventListener('click', function () {
                                const prevTr = tr.previousElementSibling;
                                if (prevTr) {
                                    tables[plugin["UIPosition"]].querySelectorAll('tbody')[0].insertBefore(tr, prevTr);
                                }
                            });
                            {
                                const i = document.createElement("i");
                                i.innerText = "arrow_upward";
                                i.setAttribute("class", "material-icons");
                                a.appendChild(i);
                            }
                            tdUp.appendChild(a);
                        }
                        tr.appendChild(tdUp);
                    }
                    {
                        const tdDown = document.createElement("td");
                        {
                            const a = document.createElement("a");
                            a.addEventListener('click', function () {
                                const nextTr = tr.nextElementSibling;
                                if (nextTr) {
                                    tables[plugin["UIPosition"]].querySelectorAll('tbody')[0].insertBefore(tr, nextTr.nextElementSibling);
                                }
                            });
                            {
                                const i = document.createElement("i");
                                i.innerText = "arrow_downward";
                                i.setAttribute("class", "material-icons");
                                a.appendChild(i);
                            }
                            tdDown.appendChild(a);
                        }
                        tr.appendChild(tdDown);
                    }
                    tables[plugin["UIPosition"]].querySelectorAll('tbody')[0].appendChild(tr);
                    // tbody.appendChild(tr);
                }
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
                    const update = [];
                    // iterate over table and construct update
                    const selectElems = modal.querySelectorAll('select');
                    for (let selectElem of selectElems) {
                        const instance = M.FormSelect.getInstance(selectElem);
                        const json = {};
                        json["name"] = selectElem.getAttribute("data-plugin-name");;
                        json["version"] = instance.getSelectedValues()[0];
                        update.push(json);
                    }
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
                i.innerText = "build";
                i.setAttribute("class", "material-icons");
            }
            li.appendChild(a);
        }
        UI.bottomMenuRightUl.appendChild(li);
    }
};

////
// UI
export const init = async function () {
    await generateUI();
}
