export const Plugin = {};

Plugin.loadPlugins = async function () {
    return APIcall("GET", "api/getPlugins").then(res => {
        var promises = [];
        var j = JSON.parse(res);
        console.log(j);
        j["plugins"].forEach((pluginPath) => {
            promises.push(
                import(pluginPath).then(
                    plugin => {
                        return plugin;
                    }
                )
            );
        });
        return Promise.all(promises);
    }).then((plugins) => {
        // sort plugins based on priority
        plugins.sort((a, b) => {
            if (a.priority < b.priority) return -1;
            if (a.priority > b.priority) return 1;
            return 0;
        });
        // generate UI (buttons, modal, etc...)
        plugins.forEach((plugin) => {
            plugin.generateUI();
        });
    });
}

Plugin.init = function () {

};
