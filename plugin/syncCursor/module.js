import * as THREE from 'https://cdn.skypack.dev/three';
import { WSTasks } from '../../js/WSTasks.js';
import { MouseKey } from '../../js/MouseKey.js';

const syncCursor = async function (parameters) {
    ////
    // [IN]
    // parameters = {
    //  "sessionUUID": sessionUUID string,
    //  "cursor": {
    //   "dir": {
    //    "x": x corrdinate of this cursor,
    //    "y": y corrdinate of this cursor
    //   },
    //   "idx": idx for cursor icon,
    //   "remove": boolean value for removing cursor for no longer connected session
    //  }
    // }

    const sessionUUID = parameters["sessionUUID"];
    if (parameters["cursor"]["remove"]) {
        document.body.removeChild(MouseKey["cursors"][sessionUUID].img);
        delete MouseKey["cursors"][sessionUUID];
    } else {
        const x = parameters["cursor"]["dir"]["x"];
        const y = parameters["cursor"]["dir"]["y"];
        const idx = parameters["cursor"]["idx"];

        if (!MouseKey["cursors"][sessionUUID]) {
            // new entry
            MouseKey["cursors"][sessionUUID] = { "dir": new THREE.Vector2(x, y), "idx": idx, "img": new Image() };
            const style = MouseKey["cursors"][sessionUUID].img.style;
            style.position = "fixed";
            style["z-index"] = "1000"; // material css sidenav has 999
            style["pointer-events"] = "none";
            MouseKey["cursors"][sessionUUID].img.src = "../icon/cursorIcon" + (idx % 10) + ".png";

            document.body.appendChild(MouseKey["cursors"][sessionUUID].img);
        }
        MouseKey["cursors"][sessionUUID]["dir"].set(x, y);

        const clientX = x + window.innerWidth / 2.0;
        const clientY = y + window.innerHeight / 2.0;
        MouseKey["cursors"][sessionUUID].img.style.left = (clientX - 16) + "px";
        MouseKey["cursors"][sessionUUID].img.style.top = (clientY - 16) + "px";
    }
}

export const init = async function () {
    WSTasks["syncCursor"] = syncCursor;
}

