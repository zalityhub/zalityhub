const fs = require('fs');
const nx = require('../util/util.js');
const Config = nx.getEnv(['mail'], true);
const stringify = require('json-stringify-safe');
const inspect = require('util').inspect;

const Imap = require('./imap.js');

const zality = Config.zality;
const auth = zality.auth;
const host = zality.host;

const imap_config = {
  user: auth.user, password: auth.password, host: host.name, port: host.port, tls: host.tls
};


function saveEmails(emails) {

for(let i = 0; i < emails.length; ++i) 
    fs.writeFileSync(`j${i}.txt`, emails[i].body.toString());
}

function parse(email) {
  let headers = [];
  let unknowns = [];
  let content = [];
  let bidValue;


  function isHeader(line) {
    let j, ch;
    for (j = 0; j < line.length; ++j) {
      ch = line.charAt(j).toLowerCase();
      if (!((ch === '-') || (ch === '_') || (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z')))
        break;
    }
    if (j < line.length && ch === ':')
      return {header: line.slice(0, j), value: line.slice(j + 1).trim()};
    return undefined;
  }


  function getBidValue(header) {
    let bid;
        if(header.header.toLowerCase().trim() === 'content-type' &&
          (bid = header.value.toLowerCase().indexOf('boundary=')) > 0 ) {
            bidValue = nx.replaceAll(header.value.slice(bid+10), ['=', '"'], ['','']);
          }
  }


  function collectHeader(lines, i) {
    for (i = i + 1; i < lines.length; ++i) {
      line = lines[i];
      if (line.length === 0)
        break;      // going into content...

      const h = isHeader(line);
      if (h || (h && h.header !== header.header)) {
        getBidValue(header);
        headers.push(header);
        if( (header = h) )
          return collectHeader(lines, i);
      }
      if (!nx.isArray(header.value))
        header.value = [header.value];
      header.value.push(line);
    }

    return i;
  }


  let header, line, i, j;
  let body = nx.CloneObj(email.body);
  const lines = body.split('\n').join('').split('\r');
  for (i = 0; i < lines.length; ++i) {
    line = lines[i];
    if (line.length === 0)
      break;      // going into content...

    header = isHeader(line);
    if (!header) {
      unknowns.push(line);
      continue;
    }

    i = collectHeader(lines, i);
    if (lines[i].length === 0)
      break;      // going into content...
  }

  if (header) {
        getBidValue(header);
    headers.push(header);
  header = undefined;
  }

  let hdrs = nx.CloneObj(headers);
  let unks = nx.CloneObj(unknowns);
  headers = [];
  unknowns = [];
  let bid_line;
  for (i = i + 1; i < lines.length; ++i) {
    line = lines[i];
    if (line.length === 0)
      continue;   // skip

    if(bidValue && line.indexOf(`=${bidValue}=`) >= 0) {
      bid_line = line;
      continue;
    }

    header = isHeader(line);
    if (!header) {
      unknowns.push(line);
      continue;
    }

    i = collectHeader(lines, i);
    if (lines[i].length === 0)
      break;      // going into content...
  }

  if (header) {
        getBidValue(header);
    headers.push(header);
  header = undefined;
  }

  content.push({headers: headers, unknowns: unknowns});
  headers = hdrs;
  unknowns = unks;
  return {seq: email.seq, boundary_id: bidValue, headers: headers, content: content, unknowns: unknowns, body: email.body};
}


const imap = new Imap.imap('main');
const p = imap.connect(imap_config);
p.then((options) => {
  console.log(`Connected: ${auth.user}:${stringify(host)}`);
  imap.fetch(options).then((options) => {
    console.log(`received ${imap.emails.length} emails`);
    imap.write_trace();
    imap.end();

    saveEmails(imap.emails);

    const parsed = parse(imap.emails[0]);
    console.log(stringify(parsed, null, 4));
  });
});
