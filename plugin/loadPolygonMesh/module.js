import { DoppelCore } from '../../../js/DoppelCore.js';
import { APIcall } from '../../../js/APIcall.js';

var text = {
    "Polygon": { "ja": "ポリゴン" },
    "Import polygon": { "ja": "ポリゴン読み込み" }
};

////
// meta information
////
export var priority = 0;

////
// UI
////
export var generateUI = function () {
    var topMenuUl = document.getElementById("topMenuUl");
    var li = document.createElement("li");
    topMenuUl.appendChild(li);

    var a = document.createElement("a");
    li.appendChild(a);
    a.innerHTML = text["polygon"][DoppelCore.language in text["polygon"] ? DoppelCore.language : "default"];
    a.addEventListener('click', function () {
        loadMeshes('.obj,.stl,.ply,.wrl');
    });
    a.setAttribute("class", "tooltipped");
    a.setAttribute("data-position", "bottom");
    a.setAttribute("data-tooltip", text["tooltip"][DoppelCore.language in text["tooltip"] ? DoppelCore.language : "default"]);

    var i = document.createElement("i");
    a.appendChild(i);
    i.innerHTML = "add";
    i.setAttribute("class", "material-icons left");

}

function loadMeshes(formatToBeAccepted) {
    // http://pirosikick.hateblo.jp/entry/2014/08/11/003235
    if (loadMeshes.inputElement) {
        document.body.removeChild(loadMeshes.inputElement);
    }

    var inputElement = loadMeshes.inputElement = document.createElement('input');

    inputElement.type = 'file';
    inputElement.multiple = 'multiple';
    inputElement.accept = formatToBeAccepted;

    inputElement.style.visibility = 'hidden';
    inputElement.style.position = 'absolute';
    inputElement.style.left = '-9999px';

    document.body.appendChild(inputElement);

    // https://www.html5rocks.com/ja/tutorials/file/dndfiles/
    // https://threejs.org/docs/#examples/loaders/GLTFLoader
    // https://qiita.com/weal/items/1a2af81138cd8f49937d
    inputElement.addEventListener('change', function (e) {
        if (e.target.files.length > 0) {
            for (var file of e.target.files) {
                loadMesh(file);
            }
        }
    }, false);
    inputElement.click();
}

function loadMesh(file) {
    var freader = new FileReader();
    freader.onload = function (event) {

        var fileName = file.name;
        var type = fileName.split('.');
        var fileId = Math.random().toString(36).substring(2, 9);

        var base64 = freader.result.substring(freader.result.indexOf(',') + 1)
        var packetSize = 1000000;
        var packetCount = Math.ceil(base64.length / packetSize);

        for (var packet = 0; packet < packetCount; ++packet) {
            var base64Packet = base64.substring(packet * packetSize, (packet + 1) * packetSize);
            var json = {
                "sessionId": DoppelCore.sessionId,
                "meshName": type.slice(0, -1).join("."),
                "fileId": fileId,
                "fileType": type[type.length - 1].toLowerCase(),
                "packetIndex": packet,
                "packetSize": packetSize,
                "packetTotal": packetCount,
                "byteTotal": base64.length,
                "base64Packet": base64Packet
            };

            APIcall("POST", "api/updatePolygonMesh", JSON.stringify(json), "application/json");
        }
    };
    freader.readAsDataURL(file);
}
