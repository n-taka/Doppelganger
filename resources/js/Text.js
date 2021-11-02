import { Core } from "./Core.js";

export const getText = function (dict, key) {
    if(!(key in dict))
    {
        return key;
    }
    if(!(Core.language in dict[key]))
    {
        return key;
    }
    return dict[key][Core.language];
};
