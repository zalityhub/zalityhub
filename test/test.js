const fs = require('fs');

const lines = fs.readFileSync('test.txt').toString().split('\n');

const servers = {};
let properties;
let pp0;


function push(p0, p1) {
  if(!properties[p0]) {
    if(p1)
      properties[p0] = p1;
    return;
  }

  if(! (properties[p0] instanceof Array)) {
    const it = properties[p0];
    properties[p0] = [];
    properties[p0].push(it);
  }

  if(p1)
    properties[p0].push(p1);
}

function depth(s, ch) {
  if(!ch)
    ch = ' ';
  let d = 0;
  while(s.charAt(d) === ch)
    ++d;
  return d;
}

for(let l = 0; l < lines.length; ++l ) {
  const line = lines[l];

  const parsed = line.trim().split(' ');
  let p0 = parsed[0];
  let p1 = parsed[1];
  const d = depth(line);
  // console.log(`d=${d}, p0=${p0}, p1=${p1}`);

  if(d === 0 && p0 === 'SERVER') {
    properties = {};
    servers[p1] = properties;
    continue;
  }

  if(d === 2 && !p1)
    pp0 = p0;

  if(d === 4) {
    p1 = line.trim();
    p0 = pp0;
  }

  push(p0, p1);
}

console.log(JSON.stringify(servers, null, 4));
