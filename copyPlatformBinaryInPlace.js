#!/usr/bin/env node

var fs = require("fs");

var arch = process.arch;
var platform = process.platform;

if (arch === "ia32") {
  arch = "x86";
}

if (platform === "win32") {
  platform = "win";
}

copyBinary("bin/bs-let-" + platform + "-" + arch + ".exe", "ppx");

function copyBinary(filename, destFilename) {
  var supported = fs.existsSync(filename);

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
    copyFileSync(filename, destFilename);
    fs.chmodSync(destFilename, 0755);
  }

  var destFilenameExe = destFilename + ".exe";
  if (!fs.existsSync(destFilenameExe)) {
    copyFileSync(filename, destFilenameExe);
    fs.chmodSync(destFilenameExe, 0755);
  }
}

function copyFileSync(source, dest) {
  if (typeof fs.copyFileSync === "function") {
    fs.copyFileSync(source, dest);
  } else {
    fs.writeFileSync(dest, fs.readFileSync(source));
  }
}
