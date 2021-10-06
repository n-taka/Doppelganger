import { UI } from '../../js/UI.js';
import { getText } from '../../js/Text.js';
import { request } from "../../js/request.js";

const text = {
    "Config editor": { "en": "Config editor", "ja": "設定" },
    "Server config": { "en": "Server config", "ja": "サーバー設定" },
    "Protocol": { "en": "Protocol", "ja": "プロトコル" },
    "Host (IP adress)": { "en": "Host (IP adress)", "ja": "ホスト (IPアドレス)" },
    "Port (0|[49152-65535])": { "en": "Port (0|[49152-65535])", "ja": "ポート (0|[49152-65535])" },
    "Browser config": { "en": "Browser config", "ja": "ブラウザ設定" },
    "Plugin config": { "en": "Plugin config", "ja": "プラグイン設定" },
    "Directory for plugin": { "en": "Directory for plugin", "ja": "プラグイン用のディレクトリ" },
    "Basically, please keep blank (directory is automatically set)": { "en": "Basically, please keep blank", "ja": "基本的には空欄のままにしてください (ディレクトリは自動で作成されます)" },
    "List of URL for plugin list (one URL per line)": { "en": "List of URL for plugin list (one URL per line)", "ja": "プラグインリストのURL (1行に1URLずつ記載してください)" },
    "Output config": { "en": "Output config", "ja": "書き出し設定" },
    "Directory for output": { "en": "Directory for output", "ja": "書き出し用のディレクトリ" },
    "Log config": { "en": "Log config", "ja": "ログ設定" },
    "Directory for log": { "en": "Directory for log", "ja": "ログ用のディレクトリ" },
    "Update & Shutdown": { "en": "Update & Shutdown", "ja": "更新して終了" },
    "Cancel": { "en": "Cancel", "ja": "キャンセル" }
};

let configJson = {};

const generateUI = async function () {
    // first, we get current config and construct a modal
    //   while constructing a modal, we implicitly assign default values
    return request("configEditor", {}).then((response) => {
        configJson = JSON.parse(response);
        console.log(configJson);
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
                    heading.innerText = getText(text, "Config editor");
                    modalContentDiv.appendChild(heading);
                }

                ////
                // server settings
                {
                    const heading = document.createElement("h6");
                    heading.innerText = getText(text, "Server config");
                    modalContentDiv.appendChild(heading);
                }
                {
                    const ul = document.createElement('ul');
                    {
                        const li = document.createElement('li');
                        {
                            const divRow = document.createElement('div');
                            divRow.setAttribute('class', 'row');
                            {
                                const divCol = document.createElement('div');
                                divCol.setAttribute('class', 'input-field col s3');
                                {
                                    const select = document.createElement('select');
                                    {
                                        const option = document.createElement('option');
                                        option.setAttribute("value", "http://");
                                        option.setAttribute('style', 'text-align: center; user-select: none;');
                                        option.setAttribute("selected", true);
                                        option.innerText = "http://";
                                        select.appendChild(option);
                                    }
                                    {
                                        const option = document.createElement('option');
                                        option.setAttribute("value", "https://");
                                        option.setAttribute('style', 'text-align: center; user-select: none;');
                                        // currently we don't support SSL
                                        option.setAttribute("disabled", true);
                                        option.innerText = "https://";
                                        select.appendChild(option);
                                    }
                                    divCol.appendChild(select);
                                }
                                {
                                    const label = document.createElement('label');
                                    label.innerText = getText(text, "Protocol");
                                    divCol.appendChild(label);
                                }
                                divRow.appendChild(divCol);
                            }
                            {
                                const divCol = document.createElement('div');
                                divCol.setAttribute('class', 'input-field col s6');
                                {
                                    const input = document.createElement('input');
                                    input.setAttribute('type', 'text');
                                    input.setAttribute('id', 'host');
                                    input.setAttribute('value', (configJson["server"] && configJson["server"]["host"]) ? configJson["server"]["host"] : '127.0.0.1');
                                    input.setAttribute('class', 'validate');
                                    input.setAttribute('style', 'text-align: center;');
                                    divCol.appendChild(input);
                                }
                                {
                                    const label = document.createElement('label');
                                    label.setAttribute('for', "host");
                                    label.innerText = getText(text, "Host (IP adress)");
                                    divCol.appendChild(label);
                                }
                                divRow.appendChild(divCol);
                            }
                            {
                                const divCol = document.createElement('div');
                                divCol.setAttribute('class', 'input-field col s3');
                                {
                                    const input = document.createElement('input');
                                    input.setAttribute('type', 'number');
                                    input.setAttribute('id', 'port');
                                    input.setAttribute('value', (configJson["server"] && configJson["server"]["port"]) ? configJson["server"]["port"] : '0');
                                    input.setAttribute('class', 'validate');
                                    input.setAttribute('min', '0');
                                    input.setAttribute('max', '65535');
                                    input.setAttribute('step', '1');
                                    input.setAttribute('style', 'text-align: center;');
                                    divCol.appendChild(input);
                                }
                                {
                                    const label = document.createElement('label');
                                    label.setAttribute('for', "port");
                                    label.innerText = getText(text, "Port (0|[49152-65535])");
                                    divCol.appendChild(label);
                                }
                                divRow.appendChild(divCol);
                            }
                            li.appendChild(divRow);
                        }
                        ul.appendChild(li);
                    }
                    modalContentDiv.appendChild(ul);
                }

                ////
                // browser settings
                {
                    const heading = document.createElement("h6");
                    heading.innerText = getText(text, "Browser config");
                    modalContentDiv.appendChild(heading);
                }
                {
                    const ul = document.createElement('ul');
                    {
                        const li = document.createElement('li');
                        {
                            // const divRow = document.createElement('div');
                            // divRow.setAttribute('class', 'row');
                            // {
                            //     const divCol = document.createElement('div');
                            //     divCol.setAttribute('class', 'input-field col s3');
                            //     {
                            //         const select = document.createElement('select');
                            //         {
                            //             const option = document.createElement('option');
                            //             option.setAttribute("value", "http://");
                            //             option.setAttribute('style', 'text-align: center; user-select: none;');
                            //             option.setAttribute("selected", true);
                            //             option.innerText = "http://";
                            //             select.appendChild(option);
                            //         }
                            //         {
                            //             const option = document.createElement('option');
                            //             option.setAttribute("value", "https://");
                            //             option.setAttribute('style', 'text-align: center; user-select: none;');
                            //             // currently we don't support SSL
                            //             option.setAttribute("disabled", true);
                            //             option.innerText = "https://";
                            //             select.appendChild(option);
                            //         }
                            //         divCol.appendChild(select);
                            //     }
                            //     {
                            //         const label = document.createElement('label');
                            //         label.innerText = getText(text, "Protocol");
                            //         divCol.appendChild(label);
                            //     }
                            //     divRow.appendChild(divCol);
                            // }
                            // {
                            //     const divCol = document.createElement('div');
                            //     divCol.setAttribute('class', 'input-field col s6');
                            //     {
                            //         const input = document.createElement('input');
                            //         input.setAttribute('type', 'text');
                            //         input.setAttribute('id', 'host');
                            //         input.setAttribute('value', (configJson["server"] && configJson["server"]["host"]) ? configJson["server"]["host"] : '127.0.0.1');
                            //         input.setAttribute('class', 'validate');
                            //         input.setAttribute('style', 'text-align: center;');
                            //         divCol.appendChild(input);
                            //     }
                            //     {
                            //         const label = document.createElement('label');
                            //         label.setAttribute('for', "host");
                            //         label.innerText = getText(text, "Host (IP adress)");
                            //         divCol.appendChild(label);
                            //     }
                            //     divRow.appendChild(divCol);
                            // }
                            // {
                            //     const divCol = document.createElement('div');
                            //     divCol.setAttribute('class', 'input-field col s3');
                            //     {
                            //         const input = document.createElement('input');
                            //         input.setAttribute('type', 'number');
                            //         input.setAttribute('id', 'port');
                            //         input.setAttribute('value', (configJson["server"] && configJson["server"]["port"]) ? configJson["server"]["port"] : 0);
                            //         input.setAttribute('value', '0');
                            //         input.setAttribute('class', 'validate');
                            //         input.setAttribute('min', '0');
                            //         input.setAttribute('max', '65535');
                            //         input.setAttribute('step', '1');
                            //         input.setAttribute('style', 'text-align: center;');
                            //         divCol.appendChild(input);
                            //     }
                            //     {
                            //         const label = document.createElement('label');
                            //         label.setAttribute('for', "port");
                            //         label.innerText = getText(text, "Port (0|[49152-65535])");
                            //         divCol.appendChild(label);
                            //     }
                            //     divRow.appendChild(divCol);
                            // }
                            // li.appendChild(divRow);
                        }
                        ul.appendChild(li);
                    }

                    {
                        const liCheckbox = document.createElement("li");
                        {
                            const pCheckbox = document.createElement("p");
                            {
                                const labelCheckbox = document.createElement("label");
                                {
                                    const inputCheckbox = document.createElement("input");
                                    inputCheckbox.setAttribute("type", "checkbox")
                                    inputCheckbox.setAttribute("id", "largestVoidOnly");
                                    inputCheckbox.checked = true;
                                    labelCheckbox.appendChild(inputCheckbox);
                                }
                                {
                                    const spanCheckbox = document.createElement("span");
                                    spanCheckbox.innerText = getText(text, "Only keep the largest void.");
                                    labelCheckbox.appendChild(spanCheckbox);
                                }
                                pCheckbox.appendChild(labelCheckbox);
                            }

                            liCheckbox.appendChild(pCheckbox);
                        }
                        ul.appendChild(liCheckbox);
                    }
                    modalContentDiv.appendChild(ul);
                }

                ////
                // plugin settings
                {
                    const heading = document.createElement("h6");
                    heading.innerText = getText(text, "Plugin config");
                    modalContentDiv.appendChild(heading);
                }
                {
                    const ul = document.createElement('ul');
                    {
                        const li = document.createElement('li');
                        {
                            const divRow = document.createElement('div');
                            divRow.setAttribute('class', 'row');
                            {
                                const divCol = document.createElement('div');
                                divCol.setAttribute('class', 'input-field col s12');
                                {
                                    const textarea = document.createElement('textarea');
                                    textarea.setAttribute('id', 'pluginListURL');
                                    textarea.setAttribute('class', 'materialize-textarea')
                                    textarea.innerText = "";
                                    if (configJson["plugin"] && configJson["plugin"]["listURL"]) {
                                        for (let url of configJson["plugin"]["listURL"]) {
                                            textarea.innerHTML += url;
                                            textarea.innerHTML += '\n';
                                        }
                                    }
                                    divCol.appendChild(textarea);
                                }
                                {
                                    const label = document.createElement('label');
                                    label.setAttribute('for', "pluginListURL");
                                    label.innerText = getText(text, "List of URL for plugin list (one URL per line)");
                                    divCol.appendChild(label);
                                }
                                divRow.appendChild(divCol);
                            }
                            li.appendChild(divRow);
                        }
                        {
                            const divRow = document.createElement('div');
                            divRow.setAttribute('class', 'row');
                            {
                                const divCol = document.createElement('div');
                                divCol.setAttribute('class', 'input-field col s12');
                                {
                                    const input = document.createElement('input');
                                    input.setAttribute('type', 'text');
                                    input.setAttribute('id', 'pluginDir');
                                    input.setAttribute('class', 'validate');
                                    input.setAttribute('placeholder', getText(text, "Basically, please keep blank (directory is automatically set)"));
                                    if (configJson["plugin"] && configJson["plugin"]["dir"] && configJson["plugin"]["dir"].length > 0) {
                                        input.setAttribute("value", configJson["plugin"]["dir"]);
                                    }
                                    divCol.appendChild(input);
                                }
                                {
                                    const label = document.createElement('label');
                                    label.setAttribute('for', "pluginDir");
                                    label.innerText = getText(text, "Directory for plugin");
                                    divCol.appendChild(label);
                                }
                                divRow.appendChild(divCol);
                            }
                            li.appendChild(divRow);
                        }
                        ul.appendChild(li);
                    }
                    modalContentDiv.appendChild(ul);
                }

                ////
                // output settings
                {
                    const heading = document.createElement("h6");
                    heading.innerText = getText(text, "Output config");
                    modalContentDiv.appendChild(heading);
                }
                {
                    const ul = document.createElement('ul');
                    {
                        const li = document.createElement('li');
                        {
                            // todo
                            // radio button

                        }
                        {
                            const divRow = document.createElement('div');
                            divRow.setAttribute('class', 'row');
                            {
                                const divCol = document.createElement('div');
                                divCol.setAttribute('class', 'input-field col s12');
                                {
                                    const input = document.createElement('input');
                                    input.setAttribute('type', 'text');
                                    input.setAttribute('id', 'outputDir');
                                    input.setAttribute('class', 'validate');
                                    input.setAttribute('placeholder', getText(text, "Basically, please keep blank (directory is automatically set)"));
                                    if (configJson["output"] && configJson["output"]["dir"] && configJson["output"]["dir"].length > 0) {
                                        input.setAttribute("value", configJson["output"]["dir"]);
                                    }
                                    divCol.appendChild(input);
                                }
                                {
                                    const label = document.createElement('label');
                                    label.setAttribute('for', "outputDir");
                                    label.innerText = getText(text, "Directory for output");
                                    divCol.appendChild(label);
                                }
                                divRow.appendChild(divCol);
                            }
                            li.appendChild(divRow);
                        }
                        ul.appendChild(li);
                    }
                    modalContentDiv.appendChild(ul);
                }

                ////
                // log settings
                {
                    const heading = document.createElement("h6");
                    heading.innerText = getText(text, "Log config");
                    modalContentDiv.appendChild(heading);
                }
                {
                    const ul = document.createElement('ul');
                    {
                        const li = document.createElement('li');
                        {
                            // todo
                            // checkboxes
                        }
                        {
                            // todo
                            // checkboxes
                        }
                        {
                            const divRow = document.createElement('div');
                            divRow.setAttribute('class', 'row');
                            {
                                const divCol = document.createElement('div');
                                divCol.setAttribute('class', 'input-field col s12');
                                {
                                    const input = document.createElement('input');
                                    input.setAttribute('type', 'text');
                                    input.setAttribute('id', 'logDir');
                                    input.setAttribute('class', 'validate');
                                    input.setAttribute('placeholder', getText(text, "Basically, please keep blank (directory is automatically set)"));
                                    if (configJson["log"] && configJson["log"]["dir"] && configJson["log"]["dir"].length > 0) {
                                        input.setAttribute("value", configJson["log"]["dir"]);
                                    }
                                    divCol.appendChild(input);
                                }
                                {
                                    const label = document.createElement('label');
                                    label.setAttribute('for', "logDir");
                                    label.innerText = getText(text, "Directory for log");
                                    divCol.appendChild(label);
                                }
                                divRow.appendChild(divCol);
                            }
                            li.appendChild(divRow);
                        }
                        ul.appendChild(li);
                    }
                    modalContentDiv.appendChild(ul);
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
                    modalFooterApplyA.innerHTML = getText(text, "Update & Shutdown");
                    modalFooterApplyA.addEventListener("click", async function () {
                        await request("configEditor", configJson);
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
                a.setAttribute("data-tooltip", getText(text, "Config editor"));
                {
                    const i = document.createElement("i");
                    a.appendChild(i);
                    i.innerText = "settings";
                    i.setAttribute("class", "material-icons");
                }
                li.appendChild(a);
            }
            UI.bottomMenuRightUl.appendChild(li);
        }
    });
};

////
// UI
export const init = async function () {
    await generateUI();
    M.textareaAutoResize(document.getElementById('pluginListURL'));
}
