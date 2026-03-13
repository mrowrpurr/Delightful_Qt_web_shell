// Applies the playwright-core patch after install.
// QtWebEngine doesn't support Browser.setDownloadBehavior — suppress the error.
import { readFileSync, writeFileSync } from "fs"

const file = "node_modules/playwright-core/lib/server/chromium/crBrowser.js"
let src = readFileSync(file, "utf8")

if (src.includes(".catch(() => {})")) {
  console.log("playwright-core already patched")
} else {
  src = src.replace("}));\n    }\n    await Promise.all(promises);", "}).catch(() => {}));\n    }\n    await Promise.all(promises);")
  writeFileSync(file, src)
  console.log("patched playwright-core (Browser.setDownloadBehavior .catch)")
}
