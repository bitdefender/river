var path = require("path");

var isDebug = (process.env["DEBUG"] == "true");
var modPath = isDebug 
    ? "./build/Debug/executionwrapper"
    : "./build/Release/executionwrapper";

console.log("Including: " + modPath);

module.exports = exports = require(modPath);
