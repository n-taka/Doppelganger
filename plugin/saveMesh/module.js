import { UI } from '../../js/UI.js';
import { getText } from '../../js/Text.js';
import { request } from '../../js/request.js';
import { constructMeshLiFromJson } from '../../js/constructMeshLiFrom.js';

const text = {
    "Choose file format to save": { "en": "Choose file format to save", "ja": "保存するフォーマットを選んでください" },
    "Cancel": { "en": "Cancel", "ja": "キャンセル" },
    "Save": { "en": "Save", "ja": "保存" }
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
                heading.innerText = getText(text, "Choose file format to save");
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
                            const objDiv = document.createElement("div");
                            objDiv.setAttribute("class", "col s3");
                            const objBtn = document.createElement("a");
                            objBtn.setAttribute("class", "waves-effect waves-light btn");
                            objBtn.setAttribute("style", "width: 100%;");
                            objBtn.innerText = "OBJ (.obj)";
                            objBtn.addEventListener('click', function () {
                                parameters["format"] = "obj";
                                request("saveMesh", parameters).then((response) => {
                                    const responseJson = JSON.parse(response);
                                    saveMesh(responseJson);
                                });
                            });
                            objDiv.appendChild(objBtn);
                            divRow.appendChild(objDiv);
                        }
                        {
                            const plyDiv = document.createElement("div");
                            plyDiv.setAttribute("class", "col s3");
                            const plyBtn = document.createElement("a");
                            plyBtn.setAttribute("class", "waves-effect waves-light btn");
                            plyBtn.setAttribute("style", "width: 100%;");
                            plyBtn.innerText = "PLY (.ply)";
                            plyBtn.addEventListener('click', function () {
                                parameters["format"] = "ply";
                                request("saveMesh", parameters).then((response) => {
                                    const responseJson = JSON.parse(response);
                                    saveMesh(responseJson);
                                });
                            });
                            plyDiv.appendChild(plyBtn);
                            divRow.appendChild(plyDiv);
                        }
                        {
                            const stlDiv = document.createElement("div");
                            stlDiv.setAttribute("class", "col s3");
                            const stlBtn = document.createElement("a");
                            stlBtn.setAttribute("class", "waves-effect waves-light btn");
                            stlBtn.setAttribute("style", "width: 100%;");
                            stlBtn.innerText = "STL (.stl)";
                            stlBtn.addEventListener('click', function () {
                                parameters["format"] = "stl";
                                request("saveMesh", parameters).then((response) => {
                                    const responseJson = JSON.parse(response);
                                    saveMesh(responseJson);
                                });
                            });
                            stlDiv.appendChild(stlBtn);
                            divRow.appendChild(stlDiv);
                        }
                        {
                            const wrlDiv = document.createElement("div");
                            wrlDiv.setAttribute("class", "col s3");
                            const wrlBtn = document.createElement("a");
                            wrlBtn.setAttribute("class", "waves-effect waves-light btn");
                            wrlBtn.setAttribute("style", "width: 100%;");
                            wrlBtn.innerText = "VRML (.wrl)";
                            wrlBtn.addEventListener('click', function () {
                                parameters["format"] = "wrl";
                                request("saveMesh", parameters).then((response) => {
                                    const responseJson = JSON.parse(response);
                                    saveMesh(responseJson);
                                });
                            });
                            wrlDiv.appendChild(wrlBtn);
                            divRow.appendChild(wrlDiv);
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
    constructMeshLiFromJson.handlers.push(
        function (json, liRoot) {
            // for element, we cannot use getElementById ...
            const pButtons = liRoot.querySelector("#buttons_" + json["UUID"]);
            {
                const a = document.createElement("a");
                a.setAttribute("class", "tooltipped");
                a.setAttribute("data-position", "top");
                a.setAttribute("data-tooltip", getText(text, "Save"));
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
                    i.innerText = "file_download";
                    a.appendChild(i);
                }
                pButtons.appendChild(a);
            }
        }
    );
}

////
// callback
const saveMesh = function (parameters) {
    const toBlob = function (base64) {
        const bin = atob(base64.replace(/^.*,/, ''));
        const buffer = new Uint8Array(bin.length);
        for (let i = 0; i < bin.length; i++) {
            buffer[i] = bin.charCodeAt(i);
        }
        try {
            const blob = new Blob([buffer.buffer], {});
            return blob;
        } catch (e) {
            return false;
        }
    }

    for (let meshUUID in parameters["meshes"]) {
        const meshJson = parameters["meshes"][meshUUID];
        for (let type in meshJson) {
            const json = meshJson[type];
            const fileName = json["fileName"] + "." + json["format"];
            const base64Str = json["base64Str"];
            // var type = "application/octet-stream";
            const file = toBlob(base64Str);
            if (window.navigator.msSaveOrOpenBlob) // IE10+
                window.navigator.msSaveOrOpenBlob(file, fileName);
            else { // Others
                const aMesh = document.createElement("a");
                const urlMesh = URL.createObjectURL(file);
                aMesh.href = urlMesh;
                aMesh.download = fileName;
                document.body.appendChild(aMesh);
                aMesh.click();
                setTimeout(function () {
                    document.body.removeChild(aMesh);
                    window.URL.revokeObjectURL(urlMesh);
                }, 0);
            }
        }
    }
}

export const init = async function () {
    await generateUI();
}

