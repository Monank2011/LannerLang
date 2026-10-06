import fs from "node:fs/promises";
import process from "node:process";
import {pathToFileURL} from "node:url";

const wasmPath = process.argv[2];
if (!wasmPath) throw new Error("usage: node test_web_loader.mjs <module.wasm>");

const originalFetch = globalThis.fetch;
globalThis.fetch = async url => {
    const value = String(url);
    if (value.startsWith("file:")) {
        const path = new URL(value).pathname;
        const data = await fs.readFile(path);
        return new Response(data, {status: 200, headers: {"Content-Type": "application/wasm"}});
    }
    return originalFetch(url);
};

const status = {
    textContent: "",
    innerHTML: "",
    classList: {add() {}, remove() {}},
    setAttribute() {},
    addEventListener() {},
    removeEventListener() {},
    focus() {},
    remove() {}
};
const button = {...status, classList:{add(){},remove(){}}, addEventListener(){}, removeEventListener(){}};
const elements = new Map([["#status", status], ["#run", button]]);

globalThis.document = {
    querySelector(selector) { return elements.get(selector) ?? null; },
    querySelectorAll(selector) { return selector === "body" ? [1] : []; }
};
globalThis.requestAnimationFrame = callback => setTimeout(() => callback(1.5), 0);
globalThis.cancelAnimationFrame = id => clearTimeout(id);

const wasmUrl = pathToFileURL(wasmPath);
const loaderUrl = new URL(wasmUrl.href.replace(/\.wasm$/, ".js"));
const {loadLanner} = await import(loaderUrl.href);
const moduleUrl = wasmUrl;
const result = await loadLanner(moduleUrl);
const code = result.exports.main();
await new Promise(resolve => setTimeout(resolve, 75));

if (code !== 1) throw new Error(`unexpected main result ${code}`);
if (status.textContent !== "Fetch callback completed") throw new Error("DOM/fetch callback was not completed");
console.log("web loader ok");
