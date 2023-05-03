const stringify = require('json-stringify-safe');

const nx = require('../util/util.js');
const Config = nx.getEnv(['chatgpt'], true);
const ChatConfig = nx.CloneObj(Config.chat);
const uuid = require('uuid');


function BuildPaths(paths) {

  const map = {};

  paths = [["options", "model_methods", "items", "davinci", "config", "hostname"]];
  paths.forEach((entry) => {
    let item;
    if(entry.length > 1) {
      item = map[entry[0]];
      if(!item)
        item = map[entry[0]] = {};
      entry = entry.slice(1);
    }
    entry.forEach((it) => {
      if(!item)
        item = {};
    });
  });

  return map;
}

let paths = nx.parseJsonFile('j.json');

paths = BuildPaths(paths);
console.log(stringify(paths));
