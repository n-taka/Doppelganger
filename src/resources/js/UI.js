import { Modal } from './Modal.js';

export const UI = {};

UI.init = async function () {
    // title
    document.title = 'Doppelganger - HTML5 GUI for geometry processing with C++ backend';

    // language setting
    {
        UI.language = (window.navigator.languages && window.navigator.languages[0]) ||
            window.navigator.language ||
            window.navigator.userLanguage ||
            window.navigator.browserLanguage;
        // we only keep first two characters
        UI.language = UI.language.substring(0, 2);
    }

    // HTML elements
    {
        // modal
        {
            UI.modalDiv = document.createElement('div');
            document.body.appendChild(UI.modalDiv);
        }
        // canvas + sidebar
        {
            UI.rootDiv = document.createElement('div');
            UI.rootDiv.setAttribute('style', 'height: 100%; width: 100%; display: flex; flex-direction: row;');
            // WebGL canvas
            {
                UI.webGLDiv = document.createElement('div');
                UI.webGLDiv.setAttribute('style', 'height: 100%; width: calc(100% - 450px); order: 0;');
                {
                    UI.webGLOutputDiv = document.createElement('div');
                    UI.webGLOutputDiv.setAttribute('style', 'height: 100%; width: 100%;');
                    UI.webGLDiv.appendChild(UI.webGLOutputDiv);
                }
                UI.rootDiv.appendChild(UI.webGLDiv);
            }
            // sidebar (sidenav)
            {
                UI.sideNavDiv = document.createElement('div');
                UI.sideNavDiv.setAttribute('style', 'height: 100%; width: 450px; order: 1; position: relative;');
                // top navbar
                {
                    UI.topNavBar = document.createElement('nav');
                    UI.topNavBar.setAttribute('class', 'nav teal lighten-2 z-depth-0');
                    UI.topNavBar.setAttribute('style', 'user-select: none;');
                    {
                        UI.topNavWrapper = document.createElement('div');
                        UI.topNavWrapper.setAttribute('class', 'nav-wrapper');
                        {
                            UI.topMenuUl = document.createElement('ul');
                            UI.topMenuUl.setAttribute('class', 'right');
                            UI.topNavWrapper.appendChild(UI.topMenuUl);
                        }
                        UI.topNavBar.appendChild(UI.topNavWrapper);
                    }
                    UI.sideNavDiv.appendChild(UI.topNavBar);
                }

                // summary navbar (bottom)
                {
                    UI.summaryNavBar = document.createElement('nav');
                    UI.summaryNavBar.setAttribute('class', 'nav teal lighten-2 z-depth-0');
                    UI.summaryNavBar.setAttribute('style', 'position: absolute; bottom: 64px; user-select: none;');
                    {
                        UI.summaryNavWrapper = document.createElement('div');
                        UI.summaryNavWrapper.setAttribute('class', 'nav-wrapper');
                        {
                            UI.summaryMenuLeftUl = document.createElement('ul');
                            UI.summaryMenuLeftUl.setAttribute('class', 'left');
                            UI.summaryNavWrapper.appendChild(UI.summaryMenuLeftUl);
                        }
                        {
                            UI.summaryMenuRightUl = document.createElement('ul');
                            UI.summaryMenuRightUl.setAttribute('class', 'right');
                            UI.summaryNavWrapper.appendChild(UI.summaryMenuRightUl);
                        }
                        UI.summaryNavBar.appendChild(UI.summaryNavWrapper);
                    }
                    UI.sideNavDiv.appendChild(UI.summaryNavBar);
                }

                // bottom navbar
                {
                    UI.bottomNavBar = document.createElement('nav');
                    UI.bottomNavBar.setAttribute('class', 'nav teal lighten-2 z-depth-0');
                    UI.bottomNavBar.setAttribute('style', 'position: absolute; bottom: 0px; user-select: none;');
                    {
                        UI.bottomNavWrapper = document.createElement('div');
                        UI.bottomNavWrapper.setAttribute('class', 'nav-wrapper');
                        {
                            UI.bottomMenuUl = document.createElement('ul');
                            UI.bottomMenuUl.setAttribute('class', 'left');
                            UI.bottomNavWrapper.appendChild(UI.bottomMenuUl);
                        }
                        UI.bottomNavBar.appendChild(UI.bottomNavWrapper);
                    }
                    UI.sideNavDiv.appendChild(UI.bottomNavBar);
                }

                // list of imported mesh
                {
                    UI.meshCollectionUl = document.createElement('ul');
                    UI.meshCollectionUl.setAttribute('class', 'collection');
                    UI.meshCollectionUl.setAttribute('style', 'max-height: calc(100% - 192px); height: calc(100% - 192px); overflow-y: scroll; margin: 0%; border:0px;');
                }

                UI.rootDiv.appendChild(UI.sideNavDiv);
            }
            document.body.appendChild(UI.rootDiv);
        }
        // busy icon
        {
            UI.busyDiv = document.createElement('div');
            UI.busyDiv.setAttribute('class', 'taskState');
            UI.busyDiv.setAttribute('style', 'visibility: hidden;');
            {
                UI.busyIconA = document.createElement('a');
                UI.busyIconA.setAttribute('class', 'taskStateText');
                UI.busyIconA.setAttribute('style', 'pointer-events: none;');
                {
                    UI.busyIconI = document.createElement('i');
                    UI.busyIconI.setAttribute('class', 'material-icons');
                    UI.busyIconI.setAttribute('style', 'font-size: 128px;');
                    UI.busyIconI.innerText = 'timer';
                    UI.busyIconA.appendChild(UI.busyIconI);
                }
                UI.busyDiv.appendChild(UI.busyIconA);
            }
            document.body.appendChild(UI.busyDiv);
        }
    }

    // utility function definitions
    {
        UI.setBusyMode = async function (mode) {
            UI.busyDiv.setAttribute('style', (mode ? 'visibility: visible;' : 'visibility: hidden;'));
        };

        // todo: add more functions
    }

    return;
};

