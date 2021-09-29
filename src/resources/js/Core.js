import { UI } from './UI.js';
import { Canvas } from './Canvas.js';
import { WS } from './WS.js';
import { MouseKey } from './MouseKey.js';
import { Modal } from './Modal.js';
import { Plugin } from './Plugin.js';

export const Core = {};

Core.init = function () {
    // unique ID for websocket connection
    Core.UUID = null;

    Core.language = (window.navigator.languages && window.navigator.languages[0]) ||
        window.navigator.language ||
        window.navigator.userLanguage ||
        window.navigator.browserLanguage;
    // we only keep first two characters
    Core.language = Core.language.substring(0, 2);

    window.onload = async function () {
        // initialize
        UI.init();
        Modal.init();
        Canvas.init();
        WS.init();
        MouseKey.init();
        await Plugin.init();
        
        await Canvas.pullCurrentMeshes();

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
    };
    return;
};

Core.init();
