const fs = require('fs');
const path = require('path');
const process = require("process");

const stringify = require('json-stringify-safe');


const dir = './deletes/';

const files = fs.readdirSync(dir);

for (let i = 0, ilen = files.length; i < ilen; ++i) {
  const fpath = path.join(dir, files[i]);
  const stat = fs.statSync(fpath);
  if (stat.isFile()) {
    const rpath = fs.realpathSync(fpath);
    fs.unlink(rpath, (err) => {
      if (err)
        throw err;
      console.log(`${rpath} deleted`);
    });
  }
}
