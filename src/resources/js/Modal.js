import { UI } from './UI.js';

export const Modal = {};

Modal.generateModal = function () {
    ////
    // error
    {
        // modal
        Modal.errorModal = document.createElement("div");
        {
            Modal.errorModal.setAttribute("class", "modal");
            // content
            {
                const modalContentDiv = document.createElement('div');
                modalContentDiv.setAttribute("class", "modal-content");
                modalContentDiv.innerHTML = `
                <h4>Oops!</h4>
                <p>
                    Some error is happening. Please debug me!
                </p>`;
                Modal.errorModal.appendChild(modalContentDiv);
            }
            // footer
            {
                const modalFooterDiv = document.createElement("div");
                modalFooterDiv.setAttribute("class", "modal-footer");
                {
                    const modalFooterCloseA = document.createElement("a");
                    modalFooterCloseA.setAttribute("class", "modal-close waves-effect waves-green btn-flat");
                    modalFooterCloseA.setAttribute("href", "#!");
                    modalFooterCloseA.innerHTML = "Close";
                    modalFooterDiv.appendChild(modalFooterCloseA);
                }
                Modal.errorModal.appendChild(modalFooterDiv);
            }
            UI.modalDiv.appendChild(Modal.errorModal);
        }
    }
};

Modal.init = async function () {
    this.generateModal();

    const modal_elems = document.querySelectorAll('.modal');
    M.Modal.init(modal_elems, {});
    return;

};

