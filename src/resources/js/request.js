import { Core } from "./Core.js";

export async function request(path, parameterJson, contentType) {
    // location.pathname.split('/')[1]: roomUUID
    const uri = location.protocol + "//" + location.host + "/" + location.pathname.split('/')[1] + "/" + path;
    const requestInfo = {};
    requestInfo["method"] = "POST";
    const payloadJson = {};
    payloadJson["sessionUUID"] = Core.UUID;
    if (parameterJson) {
        payloadJson["parameters"] = parameterJson;
    }
    else {
        payloadJson["parameters"] = {};
    }
    requestInfo["body"] = JSON.stringify(payloadJson);
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
