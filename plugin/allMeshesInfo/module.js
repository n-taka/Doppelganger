import { UI } from '../../js/UI.js';
import { Canvas } from '../../js/Canvas.js';
import { getText } from '../../js/Text.js';
import { constructMeshLiFromParameters } from '../../js/constructMeshLiFrom.js';
import * as THREE from 'https://cdn.skypack.dev/three';
import { mergeBufferGeometries } from 'https://cdn.skypack.dev/three/examples/jsm/utils/BufferGeometryUtils.js';

const text = {
    "#Triangle": { "en": "#Triangle", "ja": "ポリゴン数" }
};

////
// UI
const generateUI = async function () {
    // this li can be empty for initialization
    //   because this content is updated when Canvas.pullCurrentMeshes is called.
    const li = document.createElement("li");
    UI.summaryMenuLeftUl.appendChild(li);

    ////
    // handler
    constructMeshLiFromParameters.handlers.push(
        function () {
            const updateText = function (xSize_, ySize_, zSize_, triCount) {
                // this is not optimal ...
                li.innerText = '';
                {
                    const a = document.createElement("a");
                    a.setAttribute("style", "pointer-events:none;");
                    // size info
                    // here, we only show XX.XX
                    {
                        {
                            const xSize = document.createElement("span");
                            xSize.setAttribute("style", "background: linear-gradient(transparent 75%, #ff8080 75%);");
                            xSize.innerText = xSize_;
                            a.appendChild(xSize);
                        }
                        {
                            const x = document.createElement("span");
                            x.innerText = " × ";
                            a.appendChild(x);
                        }
                        {
                            const xSize = document.createElement("span");
                            xSize.setAttribute("style", "background: linear-gradient(transparent 75%, #80ff80 75%);");
                            xSize.innerText = ySize_;
                            a.appendChild(xSize);
                        }
                        {
                            const x = document.createElement("span");
                            x.innerText = " × ";
                            a.appendChild(x);
                        }
                        {
                            const xSize = document.createElement("span");
                            xSize.setAttribute("style", "background: linear-gradient(transparent 75%, #8080ff 75%);");
                            xSize.innerText = zSize_;
                            a.appendChild(xSize);
                        }
                    }

                    // "," for separatgor
                    {
                        const comma = document.createElement("span");
                        comma.innerText = ", ";
                        a.appendChild(comma);
                    }

                    // vertex / triangle count
                    {
                        const count = document.createElement("span");
                        count.setAttribute("style", "text-align: right;");
                        // const vertexCount = geometry.getAttribute('position').count;
                        const infoString = getText(text, "#Triangle") + ": " + triCount;
                        count.innerText = infoString;
                        a.appendChild(count);
                    }

                    li.appendChild(a);
                }
            }

            const meshList = Canvas.meshGroup.children.filter(function (obj) { return (obj instanceof THREE.Mesh); });
            if (meshList.length > 0) {
                const geometry = mergeBufferGeometries(meshList.map(function (obj) { return obj.geometry }));
                geometry.computeBoundingBox();
                const BBox = geometry.boundingBox;
                const XYZSize = BBox.max.clone();
                XYZSize.sub(BBox.min);
                updateText(XYZSize.x.toFixed(2), XYZSize.y.toFixed(2), XYZSize.z.toFixed(2), (geometry.index ? geometry.index.count / 3 : 0));
                geometry.dispose();
            } else {
                updateText("0.00", "0.00", "0.00", 0);
            }
        }
    );
}

export const init = async function () {
    await generateUI();
}

