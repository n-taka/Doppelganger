import { Modal } from "./Modal.js";

export function APIcall(path, body, contentType) {
    return new Promise((resolve, reject) => {
        const request = new XMLHttpRequest();
        // location.pathname.split('/')[1]: roomUUID
        const uri = location.protocol + "//" + location.host + "/" + location.pathname.split('/')[1] + "/" + path;
        request.open("POST", uri);
        if (contentType) {
            request.setRequestHeader("Content-Type", contentType);
        }

        request.addEventListener("load", function (event) {
            if (event.target.status !== 200) {

                const p = document.createElement("p");
                p.textContent = "Error: " + path;
                Modal.errorModal.firstElementChild.appendChild(p);
                const instance = M.Modal.getInstance(Modal.errorModal);
                instance.open();
                reject();
            }
            else {
                resolve(event.target.response);
            }
        });
        request.addEventListener("error", function () {
            console.error("Network Error");
            reject();
        });

        request.send(body);
    });
}
