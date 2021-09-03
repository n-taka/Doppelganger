////
// API call
////
//
// APIcall(...)
//

export function APIcall(method, path, body, contentType) {
    return new Promise((resolve, reject) => {
        const request = new XMLHttpRequest();
        var uri_text = location.protocol + "//" + location.host + "/" + path;
        request.open(method, uri_text);
        if (contentType) {
            request.setRequestHeader("Content-Type", contentType);
        }

        request.addEventListener("load", function (event) {
            if (event.target.status !== 200) {
                reject()
                var elem = document.getElementById("errorModal");
                var p = document.createElement("p");
                p.textContent = "Error: " + method + " " + path;
                elem.firstElementChild.appendChild(p);
                var instance = M.Modal.getInstance(elem);
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
