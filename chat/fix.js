const fs = require('fs');
const stringify = require('json-stringify-safe');
const uuid = require('uuid');
const nx = require('../util/util.js');


let history = nx.parseJsonFile('history.json');
let o = {};
let grouped = [];

for (let i = 0; i < history.length; ++i)
  o[history[i].uuid] = history[i];

for (const [key, value] of Object.entries(nx.CloneObj(o))) {
  const k = key.slice(key.indexOf('_') + 1);
  const g = {uuid: k};
  let h = o['user_' + k];   // the user item
  if (h) {
    g.user = h.content;
    delete o[h.uuid];
  }
  h = o['bot_' + k];   // the bot item
  if (h) {
    g.bot = h.content;
    delete o[h.uuid];
  }
  if (Object.entries(g).length > 2)
    grouped.push(g);
  else if (Object.entries(g).length !== 1)
    console.log(`Orphaned: ${stringify(g)}`);
}

let q;
o = {};
history = fs.readFileSync('j').toString().split('\n');
for (let i = 0; i < history.length; ++i) {
  let h = history[i].trim();
  if (h.indexOf('Question:') === 0) {
    h = h.slice(h.indexOf(':') + 1).trim();
    if (q === h)
      continue;         // repeat
    q = h;            // save
    continue;
  }
  if (h.indexOf('Response:') === 0) {
    h = h.slice(h.indexOf(':') + 1).trim();
    if (nx.isNull(q)) {        // no matching question?
      console.log('No question for: ' + h);
      continue;
    }
    const g = {uuid: uuid.v4()};
    g.user = q;
    g.bot = h;
    grouped.push(g);
    q = null;
  }
}

/*
Question: have we spoken of flashlights
Question: have we spoken of flashlights
Question: have we spoken of flashlights
Question: have we spoken of flashlights
Question: have we spoken of flashlights
Response: Yes, you asked when the first commercially available flashlight was available. The first commercially available flashlight was invented in 1899 by British inventor David Misell. It was a handheld device that used three D batteries and a small incandescent light bulb.
*/

console.log(stringify(grouped, null, 4));
