import * as THREE from 'https://cdn.skypack.dev/three@v0.132';
import { Canvas } from '../../js/Canvas.js';
import { UI } from '../../js/UI.js';
import { getText } from '../../js/Text.js';

const text = {
    "Toggle Axes": { "en": "Toggle Axes", "ja": "座標軸表示/非表示" }
};

const generateUI = function () {
    ////
    // button
    {
        const li = document.createElement("li");
        {
            const a = document.createElement("a");
            a.addEventListener('click', function () {
                toggleAxes();
            });
            a.setAttribute("class", "tooltipped");
            a.setAttribute("data-position", "top");
            a.setAttribute("data-tooltip", getText(text, "Toggle Axes"));
            {
                const i = document.createElement("i");
                a.appendChild(i);
                i.innerText = "straighten";
                i.setAttribute("class", "material-icons");
            }
            li.appendChild(a);
        }
        UI.bottomMenuLeftUl.appendChild(li);
    }
}

const toggleAxes = function () {
    Canvas.axes.visible = !Canvas.axes.visible;
}

export const init = async function () {
    // create axes
    Canvas.axes = new THREE.AxesHelper(100);
    Canvas.axes.visible = true;
    Canvas.predefGroup.add(Canvas.axes);
    // generate UI
    generateUI();
}
