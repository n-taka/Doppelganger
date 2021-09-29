import { UI } from './UI.js';

////
// [IN]
// parameters = {
//  "meshes": {
//   "<UUID>": {
//    "UUID": UUID of this mesh,
//    "remove" (optional): boolean flag for mesh remove,
//    "name": name of this mesh,
//    "visibility": visibility of this mesh,
//    "V": base64-encoded vertices (#V),
//    "F": base64-encoded facets (#F),
//    "VC": base64-encoded vertex colors (#V),
//    "TC": base64-encoded texture coordinates (#V),
//    "FC": base64-encoded vertices (#F, only for edit history),
//    "FTC": base64-encoded vertices (#F, only for edit history),
//    "textures": [
//     {
//      "name": original filename for this texture
//      "width" = width of this texture
//      "height" = height of this texture
//      "texData" = base64-encoded texture data
//     }
//    ]
//   },
//   ...
//  }
// }
// 
// [OUT]
// nothing

export const constructMeshLiFromParameters = async function (parameters) {
    if ("meshes" in parameters) {
        // use document fragment to optimize the performance
        const collectionFrag = document.createDocumentFragment();
        for (let meshUUID in parameters["meshes"]) {
            // erase old meshLi (not optimal...)
            if (meshUUID in UI.UUIDToMeshLi) {
                const meshLi = UI.UUIDToMeshLi[meshUUID];
                // remove from Li list
                UI.meshCollectionUl.removeChild(meshLi)
                // remove from map
                delete UI.UUIDToMeshLi[meshUUID];
                // explicitly destroy tooltip (for avoiding zombie elements)
                const tooltippedElements = meshLi.querySelectorAll('.tooltipped');
                for (let element of tooltippedElements) {
                    const tooltipInstance = M.Tooltip.getInstance(element);
                    tooltipInstance.destroy();
                }
            }
            if (!parameters["meshes"][meshUUID]["remove"]) {
                const updatedMeshLi = await constructMeshLiFromJson(parameters["meshes"][meshUUID]);
                collectionFrag.appendChild(updatedMeshLi);
                UI.UUIDToMeshLi[meshUUID] = updatedMeshLi;
            }
        }
        UI.meshCollectionUl.appendChild(collectionFrag);
    }

    for(let handler of constructMeshLiFromParameters.handlers)
    {
        await handler();
    }

    //     for (var c of meshCollection.children) {
    //         meshCollectionFrag.appendChild(c.cloneNode(true));

    //         // manually clone handlers
    //         DoppelCore.toolHandlerGenerator.forEach((generator) => {
    //             if (generator.hasOwnProperty("id")) {
    //                 var doppelIdSub = parseInt(c.id.substring(4));
    //                 var aPlugin = meshCollectionFrag.getElementById("a" + generator.id + doppelIdSub);
    //                 aPlugin.onclick = function () {
    //                     DoppelCore.selectedDoppelId = [doppelIdSub];
    //                     generator.handler();
    //                 };
    //             }
    //         });

    //     }
}
// handlers that need to be called when we call constructMeshLiFromParameters
// function (void) { ... }
constructMeshLiFromParameters.handlers = [];

////
// [IN]
// json = {
//  "UUID": UUID of this mesh,
//  "name": name of this mesh,
//  "visibility": visibility of this mesh,
//  "V": base64-encoded vertices (#V),
//  "F": base64-encoded facets (#F),
//  "VC": base64-encoded vertex colors (#V),
//  "TC": base64-encoded texture coordinates (#V),
//  "FC": base64-encoded vertices (#F, only for edit history),
//  "FTC": base64-encoded vertices (#F, only for edit history),
//  "textures": [
//   {
//    "name": original filename for this texture
//    "width" = width of this texture
//    "height" = height of this texture
//    "texData" = base64-encoded texture data
//   }
//  ]
// }
// 
// [OUT]
// <li> element that corresponds to the mesh

export const constructMeshLiFromJson = async function (json) {
    // using document fragment to optimize the performance
    const liRoot = document.createElement("li");
    liRoot.setAttribute("class", "collection-item avatar");
    liRoot.setAttribute("id", "mesh_" + json["UUID"]);
    {
        // icon (empty)
        const iIcon = document.createElement("i");
        iIcon.setAttribute("class", "material-icons circle lighten-2");
        iIcon.setAttribute("style", "user-select: none;");
        iIcon.innerText = "";
        // iIcon.setAttribute("id", "icon_" + json["UUID"]);
        liRoot.appendChild(iIcon);
    }
    {
        // mesh title
        const spanTitle = document.createElement("span");
        spanTitle.setAttribute("class", "title");
        // spanTitle.setAttribute("id", "title_" + json["UUID"]);
        spanTitle.innerText = json["name"];
        liRoot.appendChild(spanTitle)
    }
    {
        // notification
        const aNotify = document.createElement("a");
        aNotify.setAttribute("class", "secondary-content");
        aNotify.setAttribute("style", "user-select: none;");
        // aNotify.setAttribute("id", "notify_" + json["UUID"]);
        liRoot.appendChild(aNotify);
    }
    {
        // meta information
        const divMetaInfo = document.createElement("div");
        divMetaInfo.setAttribute("id", "metaInfo_" + json["UUID"]);

        // for empty content, we add <br> for better layout
        const pDummy = document.createElement("p");
        divMetaInfo.appendChild(pDummy);
        pDummy.innerHTML = "<br>";

        liRoot.appendChild(divMetaInfo);
    }
    {
        // buttons
        const pButtons = document.createElement("p");
        pButtons.setAttribute("style", "text-align: right; user-select: none;");
        pButtons.setAttribute("id", "buttons_" + json["UUID"]);
        liRoot.appendChild(pButtons);
    }

    for(let handler of constructMeshLiFromJson.handlers)
    {
        await handler(json, liRoot);
    }

    return liRoot;
}
// handlers that need to be called when we call constructMeshLiFromJson
// function (json, liRoot) { ... }
constructMeshLiFromJson.handlers = [];

// handler example...
// DoppelCore.toolHandlerGenerator.forEach((generator) => {
//     var aPlugin = document.createElement("a");
//     var iPlugin = document.createElement("i");
//     iPlugin.setAttribute("class", "material-icons teal-text text-lighten-2");
//     iPlugin.innerHTML = generator.icon;
//     if (generator.hasOwnProperty("id")) {
//         aPlugin.setAttribute("id", "a" + generator.id + doppelId);
//         iPlugin.setAttribute("id", "i" + generator.id + doppelId);
//     }
//     aPlugin.appendChild(iPlugin);
//     aPlugin.onclick = function () {
//         DoppelCore.selectedDoppelId = [doppelId];
//         generator.handler();
//     };
//     if (generator.hasOwnProperty("tooltip")) {
//         aPlugin.setAttribute("class", "tooltipped");
//         aPlugin.setAttribute("data-position", "top");
//         aPlugin.setAttribute("data-tooltip", generator["tooltip"]);
//     }

//     pButtons.appendChild(aPlugin);
// });
