import { request } from "./request.js";

export const Plugin = {};

Plugin.loadPlugin = async function (name, version) {
    // http://example.com/<roomUUID>/plugin/APIName_version/module.js
    const path = "../plugin/" + name + "_" + version + "/module.js";
    const module = await import(path);
    await module.init();
}

Plugin.init = async function () {
    Plugin.pluginList = JSON.parse(await request("listPlugins", {}));
    for (let plugin of Plugin.pluginList) {
        if (plugin["installedVersion"].length > 0 && plugin["hasModuleJS"]) {
            // we explicitly load plugin sequentially
            await Plugin.loadPlugin(plugin["name"], plugin["installedVersion"] == "latest" ? plugin["versions"][0]["version"] : plugin["installedVersion"]);
        }
    }

    // we take care of UI components that plugins generated.
    const modal_elems = document.querySelectorAll('.modal');
    M.Modal.init(modal_elems, {});
    const dropdown_elems = document.querySelectorAll('.dropdown-trigger');
    M.Dropdown.init(dropdown_elems, {});
    const tooltip_elems = document.querySelectorAll('.tooltipped');
    M.Tooltip.init(tooltip_elems, {});
    const select_elems = document.querySelectorAll('select');
    M.FormSelect.init(select_elems, {});
    M.updateTextFields();
};
