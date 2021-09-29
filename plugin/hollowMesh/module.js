import { UI } from '../../js/UI.js';
import { WSTasks } from '../../js/WSTasks.js';
import { getText } from '../../js/Text.js';
import { request } from '../../js/request.js';
import { constructMeshFromParameters } from '../../js/constructMeshFrom.js';
import { constructMeshLiFromParameters, constructMeshLiFromJson } from '../../js/constructMeshLiFrom.js';

const text = {
    "Hollow settings": { "en": "Hollow settings", "ja": "中空化設定" },
    "Hollow": { "en": "Hollow", "ja": "中空化" },
    "Shell thickness [mm]": { "en": "Shell thickness [mm]", "ja": "シェルの肉厚 [mm]" },
    "Voxel size for remesh": { "en": "Voxel size for remesh", "ja": "リメッシュの際のボクセルサイズ" },
    "Only keep the largest void.": { "en": "Only keep the largest void.", "ja": "最も体積の大きな内部空洞だけ保持する。" },
    "Cancel": { "en": "Cancel", "ja": "キャンセル" }
};

const parameters = {};

////
// UI
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
                heading.innerText = getText(text, "Hollow settings");
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
                            divCol.setAttribute('class', 'input-field col s6');
                            {
                                const input = document.createElement('input');
                                input.setAttribute('type', 'number');
                                input.setAttribute('id', 'shellThickness');
                                input.setAttribute('value', '3.0');
                                input.setAttribute('class', 'validate');
                                input.setAttribute('min', '0.01');
                                input.setAttribute('max', '100.0');
                                input.setAttribute('step', '0.0001');
                                input.setAttribute('style', 'text-align: center;');
                                divCol.appendChild(input);
                            }
                            {
                                const label = document.createElement('label');
                                label.setAttribute('for', "shellThickness");
                                label.innerText = getText(text, "Shell thickness [mm]");
                                divCol.appendChild(label);
                            }
                            divRow.appendChild(divCol);
                        }
                        {
                            const divCol = document.createElement('div');
                            divCol.setAttribute('class', 'input-field col s6');
                            {
                                const input = document.createElement('input');
                                input.setAttribute('type', 'number');
                                input.setAttribute('id', 'oneVoxelSize');
                                input.setAttribute('value', '1.0');
                                input.setAttribute('class', 'validate');
                                input.setAttribute('min', '0.01');
                                input.setAttribute('max', '100.0');
                                input.setAttribute('step', '0.0001');
                                input.setAttribute('style', 'text-align: center;');
                                divCol.appendChild(input);
                            }
                            {
                                const label = document.createElement('label');
                                label.setAttribute('for', "oneVoxelSize");
                                label.innerText = getText(text, "Voxel size for remesh");
                                divCol.appendChild(label);
                            }
                            divRow.appendChild(divCol);
                        }
                        li.appendChild(divRow);
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
                modalFooterApplyA.innerHTML = getText(text, "Hollow");
                modalFooterApplyA.addEventListener("click", async function () {
                    const oneVoxelSizeInput = document.getElementById("oneVoxelSize");
                    let oneVoxelSize = parseFloat(oneVoxelSizeInput.value);
                    if (Number.isNaN(oneVoxelSize)) {
                        oneVoxelSize = 1.0;
                        oneVoxelSizeInput.value = 1.0;
                    }
                    const shellThicknessInput = document.getElementById("shellThickness");
                    let shellThickness = parseFloat(shellThicknessInput.value);
                    if (Number.isNaN(shellThickness)) {
                        shellThickness = 3.0;
                        shellThicknessInput.value = 3.0;
                    }
                    parameters["oneVoxelSize"] = oneVoxelSize;
                    parameters["shellThickness"] = shellThickness;
                    parameters["largestVoidOnly"] = document.getElementById('largestVoidOnly').checked;
                    request("hollowMesh", parameters);
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
    // handler
    constructMeshLiFromJson.handlers.push(
        function (json, liRoot) {
            // for element, we cannot use getElementById ...
            const pButtons = liRoot.querySelector("#buttons_" + json["UUID"]);
            {
                const a = document.createElement("a");
                a.setAttribute("class", "tooltipped");
                a.setAttribute("data-position", "top");
                a.setAttribute("data-tooltip", getText(text, "Hollow"));
                a.addEventListener('click', function (e) {
                    parameters["meshes"] = [json["UUID"]];
                    const instance = M.Modal.getInstance(modal);
                    instance.open();
                    // don't fire click event on the parent (e.g. outlineOnClick)
                    e.stopPropagation();
                });
                const instance = M.Tooltip.init(a, {});

                {
                    const i = document.createElement("i");
                    i.setAttribute("class", "material-icons teal-text text-lighten-2");
                    i.innerText = "select_all";
                    a.appendChild(i);
                }
                pButtons.appendChild(a);
            }
        }
    );
}

////
// WS API
const hollowMesh = async function (parameters) {
    await constructMeshFromParameters(parameters);
    await constructMeshLiFromParameters(parameters);
}

export const init = async function () {
    await generateUI();
    WSTasks["hollowMesh"] = hollowMesh;
}

