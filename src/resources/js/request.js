import { Core } from "./Core.js";

export async function request(path, bodyJson, contentType) {
    // location.pathname.split('/')[1]: roomUUID
    const uri = location.protocol + "//" + location.host + "/" + location.pathname.split('/')[1] + "/" + path;
    const requestInfo = {};
    requestInfo["method"] = "POST";
    if (!bodyJson) {
        bodyJson = {};
    }
    if (!bodyJson["sessionUUID"]) {
        bodyJson["sessionUUID"] = Core.UUID;
    }
    requestInfo["body"] = JSON.stringify(bodyJson);
    if (contentType) {
        requestInfo["headers"] = {};
        requestInfo["headers"]["Content-Type"] = contentType;
    }
    return fetch(uri, requestInfo).then(async response => {
        if (response.ok) {
            return response.text();
        }
        else {
            throw new Error(`Request failed: ${response.status}`);
        }
    });
}
