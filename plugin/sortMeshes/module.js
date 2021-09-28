import { UI } from '../../js/UI.js';
import { Canvas } from '../../js/Canvas.js';
import { getText } from '../../js/Text.js';
import { constructMeshLiFromParameters } from '../../js/constructMeshLiFrom.js';

var text = {
    "Sort": { "default": "Sort", "ja": "並べ替え" },
    "Name": { "default": "Name", "ja": "名前" },
    "#Triangle": { "default": "#Triangle", "ja": "ポリゴン数" }
};

const generateUI = function () {
    ////
    // button
    {
        // li
        const li = document.createElement("li");
        li.setAttribute("class", "dropdown-trigger")
        li.setAttribute("data-target", "dropdownSort")
        {
            const a = document.createElement("a");
            a.setAttribute("class", "tooltipped");
            a.setAttribute("data-position", "bottom");
            a.setAttribute("data-tooltip", getText(text, "Sort"));
            {
                const i = document.createElement("i");
                i.innerText = "sort";
                i.setAttribute("class", "material-icons");
                a.appendChild(i);
            }
            li.appendChild(a);
        }
        UI.topMenuRightUl.appendChild(li);

        // dropdown
        {
            const dropdownContentUl = document.createElement('ul');
            dropdownContentUl.setAttribute("id", "dropdownSort");
            dropdownContentUl.setAttribute("class", "dropdown-content");
            {
                const nameLi = document.createElement('li');
                {
                    const nameA = document.createElement('a');
                    nameA.innerHTML = getText(text, "Name");
                    nameLi.appendChild(nameA);
                }
                nameLi.addEventListener("click", function () {
                    sortMeshes.sortMode = "name";
                    sortMeshes();
                });
                dropdownContentUl.appendChild(nameLi);
            }

            {
                const triLi = document.createElement('li');
                {
                    const triA = document.createElement('a');
                    triA.innerHTML = getText(text, "#Triangle");
                    triLi.appendChild(triA);
                }
                triLi.addEventListener("click", function () {
                    sortMeshes.sortMode = "triangle";
                    sortMeshes();
                });
                dropdownContentUl.appendChild(triLi);
            }
            UI.dropdownDiv.appendChild(dropdownContentUl);
        }
    }

    ////
    // handler
    constructMeshLiFromParameters.handlers.push(
        function () {
            sortMeshes();
        }
    );
}

const sortMeshes = function () {
    const comp = function (a, b) {
        // a, b: li element
        // id: "mesh_<UUID>"
        const aUUID = a.id.substring(5);
        const bUUID = b.id.substring(5);
        const aMesh = Canvas.UUIDToMesh[aUUID];
        const bMesh = Canvas.UUIDToMesh[bUUID];

        if (sortMeshes.sortMode == "name") {
            const aMeshName = aMesh.name;
            const bMeshName = bMesh.name;
            return aMeshName.localeCompare(bMeshName);
        } else if (sortMeshes.sortMode == "triangle") {
            const aTriangleCount = aMesh.geometry.index.count / 3;
            const bTriangleCount = bMesh.geometry.index.count / 3;
            return (aTriangleCount - bTriangleCount);
        }
    }

    const meshCollection = UI.meshCollectionUl;
    const meshCollectionArray = Array.prototype.slice.call(meshCollection.children);
    meshCollectionArray.sort(comp);
    for (var i = 0; i < meshCollectionArray.length; ++i) {
        meshCollection.appendChild(meshCollection.removeChild(meshCollectionArray[i]));
    }
}
sortMeshes.sortMode = "name";

export const init = async function () {
    generateUI();
}

