import { UI } from './UI.js';
import { Canvas } from './Canvas.js';
import { WS } from './WS.js';

// import { DoppelWS } from './websocket.js';
// import { syncMeshes } from './websocketCallBackFuncs.js';
// import { modal } from './setupModal.js';
// import { APIcall } from './APIcall.js';
// import { mouseKey } from './mouseKey.js';

export var Core = {};

Core.init = function () {
    // unique ID for websocket connection
    Core.sessionId = null;
    Core.strokeTimeStamp = null;

    Core.selectedMeshUUID = [];

    Core.language = (window.navigator.languages && window.navigator.languages[0]) ||
        window.navigator.language ||
        window.navigator.userLanguage ||
        window.navigator.browserLanguage;
    // we only keep first two characters
    Core.language = Core.language.substring(0, 2);

    window.onload = function () {
        // initialize UI
        UI.init();
        Canvas.init();
        WS.init();

        // mouseKey.init();

        // APIcall("GET", "api/serverInfo").then((res) => {
        //     var j = JSON.parse(res);
        //     DoppelCore.hostOS = j["hostOS"];
        //     DoppelCore.ChromeInstalled = j["ChromeInstalled"];
        //     DoppelCore.FreeCADInstalled = j["FreeCADInstalled"];

        //     return DoppelCore.loadPlugins();
        // }).then(() => {
        //     DoppelCore.toolHandler.push(async function (meshCollectionFrag, doppelId) {
        //         var iIcon = meshCollectionFrag.getElementById("icon" + doppelId);
        //         if (iIcon.innerText == "") {
        //             iIcon.innerText += "check";
        //             iIcon.classList.add("teal");
        //         } else {
        //             iIcon.classList.add("orange");
        //         }
        //     });

        //     modal.generate();
        //     modal.init();

        //     if (!DoppelCore.ChromeInstalled) {
        //         // open modal for telling the our suggested browser is Chrome
        //         var elem = document.getElementById("ChromeInstallModal");
        //         var instance = M.Modal.getInstance(elem); instance.open();
        //     }

        //     // slider
        //     var slider = document.getElementById('clippingSlider');
        //     noUiSlider.create(slider, {
        //         start: [0, 100],
        //         connect: [false, true, false],
        //         step: 1,
        //         direction: 'rtl',
        //         tooltips: false,
        //         orientation: 'vertical', // 'horizontal' or 'vertical'
        //         range: {
        //             'min': [0],
        //             'max': [100]
        //         }
        //     });
        //     slider.noUiSlider.on('update', function () {
        //         resetCamera();
        //     });
        //     slider.noUiSlider.on('end', function () {
        //         document.body.style.cursor = "url(../icon/cursorIcon" + (DoppelCore.sessionId % 10) + ".png) 16 16 , default"
        //     });

        //     APIcall("GET", "api/currentMeshes").then((res) => {
        //         var j = JSON.parse(res);
        //         syncMeshes({ "meshes": j });
        //     });
        // });

        // setInterval(function () {
        //     DoppelCore.periodicHandler.forEach((handler) => handler());
        // }, 1000);
    };
};

Core.init();