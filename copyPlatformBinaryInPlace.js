#!/usr/bin/env node

var fs = require("fs");
var path = require("path");

var arch = process.arch === "ia32" ? "x86" : process.arch;
var platform = process.platform === "win32" ? "win" : process.platform;

var rootDir = __dirname;
var sourceExePath = path.join(rootDir, `bs-let-${platform}-${arch}.exe`);
var destExePath = path.join(rootDir, `ppx`);

copyBinary(sourceExePath, destExePath);

function copyBinary(sourceFilename, destFilename) {
  var supported = fs.existsSync(sourceFilename);

  if (!supported) {
    console.error("bs-let does not support this platform :(");
    console.error("");
    console.error("bs-let comes prepacked as built binaries to avoid large");
    console.error("dependencies at build-time.");
    console.error("");
    console.error("If you want bs-let to support this platform natively,");
    console.error(
      "please open an issue at our repository, linked above. Please"
    );
    console.error("specify that you are on the " + platform + " platform,");
    console.error("on the " + arch + " architecture.");
  }

  if (!fs.existsSync(destFilename)) {
    copyFileSync(sourceFilename, destFilename);
    fs.chmodSync(destFilename, 0755);
  }
}

function copyFileSync(source, dest) {
  if (typeof fs.copyFileSync === "function") {
    fs.copyFileSync(source, dest);
  } else {
    fs.writeFileSync(dest, fs.readFileSync(source));
  }
}
