////
// function for setup modal
// In addition, init() set proper handlers for buttons on navbar
////

export const modal = {};

modal.generate = function () {
    var modalSet = document.getElementById('modalDiv');

    ////
    // Chrome
    ////
    {
        var modalContentDiv = document.createElement('div');
        modalContentDiv.setAttribute("class", "modal-content");
        modalContentDiv.innerHTML = `
            <ul>
                <h5>Please use Google Chrome</h5>
                <li>
                Our system works best with Google Chrome. Please visit their web page and install it.
                </li>
                <li>
                <a href="https://www.google.com/chrome/" target="_blank" rel="noopener noreferrer">https://www.google.com/chrome/</a>
                </li>
                <li>
                After the installation, please visit <a href="http://localhost:34568/" target="_blank" rel="noopener noreferrer">http://localhost:34568/</a>.
                </li>
            </ul>`;

        // footer
        var modalFooterDiv = document.createElement("div");
        modalFooterDiv.setAttribute("class", "modal-footer");

        var modalFooterCloseA = document.createElement("a");
        modalFooterCloseA.setAttribute("class", "modal-close waves-effect waves-green btn-flat");
        modalFooterCloseA.setAttribute("href", "#!");
        modalFooterCloseA.innerHTML = "Close";
        modalFooterDiv.appendChild(modalFooterCloseA);

        // modal
        var modalDiv = document.createElement("div");
        modalDiv.appendChild(modalContentDiv);
        modalDiv.appendChild(modalFooterDiv);
        modalDiv.setAttribute("id", "ChromeInstallModal");
        modalDiv.setAttribute("class", "modal");

        modalSet.appendChild(modalDiv);
    }

    ////
    // error
    ////
    {
        var modalContentDiv = document.createElement('div');
        modalContentDiv.setAttribute("class", "modal-content");
        modalContentDiv.innerHTML = `
        <h4>Oops!</h4>
        <p>
            Some error is happening. Please debug me!
        </p>`;

        // footer
        var modalFooterDiv = document.createElement("div");
        modalFooterDiv.setAttribute("class", "modal-footer");

        var modalFooterCloseA = document.createElement("a");
        modalFooterCloseA.setAttribute("class", "modal-close waves-effect waves-green btn-flat");
        modalFooterCloseA.setAttribute("href", "#!");
        modalFooterCloseA.innerHTML = "Close";
        modalFooterDiv.appendChild(modalFooterCloseA);

        // modal
        var modalDiv = document.createElement("div");
        modalDiv.appendChild(modalContentDiv);
        modalDiv.appendChild(modalFooterDiv);
        modalDiv.setAttribute("id", "errorModal");
        modalDiv.setAttribute("class", "modal");

        modalSet.appendChild(modalDiv);
    }

};

modal.init = function () {
    // because this function is called from window.onload, we should not use DOMContentLoaded
    // window.addEventListener('DOMContentLoaded', function () {
    var modal_elems = document.querySelectorAll('.modal');
    var modal_instances = M.Modal.init(modal_elems, {});
    var dropdown_elems = document.querySelectorAll('.dropdown-trigger');
    var dropdown_instances = M.Dropdown.init(dropdown_elems, {});
    var tooltip_elems = document.querySelectorAll('.tooltipped');
    var tooltip_instances = M.Tooltip.init(tooltip_elems, {});
    // });
};

