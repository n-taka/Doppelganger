import { canvas, resetCamera } from './canvas.js';
import { DoppelWS } from './websocket.js';
import { syncMeshes } from './websocketCallBackFuncs.js';
import { modal } from './setupModal.js';
import { APIcall } from './APIcall.js';
import { mouseKey } from './mouseKey.js';


export var DoppelCore = {};

DoppelCore.loadPlugins = function () {
    return APIcall("GET", "api/getPlugins").then(res => {
        var promises = [];
        var j = JSON.parse(res);
        console.log(j);
        j["plugins"].forEach((pluginPath) => {
            promises.push(
                import(pluginPath).then(
                    plugin => {
                        return plugin;
                    }
                )
            );
        });
        return Promise.all(promises);
    }).then((plugins) => {
        // sort plugins based on priority
        plugins.sort((a, b) => {
            if (a.priority < b.priority) return -1;
            if (a.priority > b.priority) return 1;
            return 0;
        });
        // generate UI (buttons, modal, etc...)
        plugins.forEach((plugin) => {
            plugin.generateUI();
        });
    });
}

DoppelCore.init = function () {

    // unique ID for websocket connection
    DoppelCore.sessionId = -1;
    DoppelCore.strokeTimeStamp = 0;

    DoppelCore.selectedDoppelId = [];

    DoppelCore.language = (window.navigator.languages && window.navigator.languages[0]) ||
        window.navigator.language ||
        window.navigator.userLanguage ||
        window.navigator.browserLanguage;
    // we only keep first two characters
    DoppelCore.language = DoppelCore.language.substring(0, 2);

    document.title = "Doppel - HTML5 GUI for geometry processing with C++ backend";

    DoppelCore.toolHandlerGenerator = [];
    DoppelCore.toolHandler = [];
    DoppelCore.systemHandler = [];
    DoppelCore.periodicHandler = [];

    DoppelCore.toolHandler.push(async function (meshCollectionFrag, doppelId) {
        var iIcon = meshCollectionFrag.getElementById("icon" + doppelId);
        iIcon.classList.remove("orange");
        iIcon.classList.remove("teal");
        iIcon.innerText = "";
    });

    window.onload = function () {
        DoppelWS.init();

        canvas.init();
        mouseKey.init();

        APIcall("GET", "api/serverInfo").then((res) => {
            var j = JSON.parse(res);
            DoppelCore.hostOS = j["hostOS"];
            DoppelCore.ChromeInstalled = j["ChromeInstalled"];
            DoppelCore.FreeCADInstalled = j["FreeCADInstalled"];

            return DoppelCore.loadPlugins();
        }).then(() => {
            DoppelCore.toolHandler.push(async function (meshCollectionFrag, doppelId) {
                var iIcon = meshCollectionFrag.getElementById("icon" + doppelId);
                if (iIcon.innerText == "") {
                    iIcon.innerText += "check";
                    iIcon.classList.add("teal");
                } else {
                    iIcon.classList.add("orange");
                }
            });

            modal.generate();
            modal.init();

            if (!DoppelCore.ChromeInstalled) {
                // open modal for telling the our suggested browser is Chrome
                var elem = document.getElementById("ChromeInstallModal");
                var instance = M.Modal.getInstance(elem); instance.open();
            }

            // slider
            var slider = document.getElementById('clippingSlider');
            noUiSlider.create(slider, {
                start: [0, 100],
                connect: [false, true, false],
                step: 1,
                direction: 'rtl',
                tooltips: false,
                orientation: 'vertical', // 'horizontal' or 'vertical'
                range: {
                    'min': [0],
                    'max': [100]
                }
            });
            slider.noUiSlider.on('update', function () {
                resetCamera();
            });
            slider.noUiSlider.on('end', function () {
                document.body.style.cursor = "url(../icon/cursorIcon" + (DoppelCore.sessionId % 10) + ".png) 16 16 , default"
            });

            APIcall("GET", "api/currentMeshes").then((res) => {
                var j = JSON.parse(res);
                syncMeshes({ "meshes": j });
            });
        });

        setInterval(function () {
            DoppelCore.periodicHandler.forEach((handler) => handler());
        }, 1000);
    };

    window.addEventListener('resize', function () {
        // var aspect = canvas.width / canvas.height;
        canvas.width = document.getElementById("WebGLDiv").offsetWidth;
        canvas.height = document.getElementsByTagName("body")[0].offsetHeight;

        canvas.camera.left = - canvas.width / 2;
        canvas.camera.right = canvas.width / 2;
        canvas.camera.top = canvas.height / 2;
        canvas.camera.bottom = - canvas.height / 2;
        // canvas.camera.aspect = canvas.width / canvas.height;
        canvas.camera.updateProjectionMatrix();

        canvas.renderer.setPixelRatio(window.devicePixelRatio);
        canvas.renderer.setSize(canvas.width, canvas.height);
        canvas.effectComposer.setSize(canvas.width, canvas.height);
    });
};

DoppelCore.init();
