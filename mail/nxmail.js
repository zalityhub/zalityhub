exports.stringify = require('json-stringify-safe');
exports.nx = require('../util/util.js');

exports.Smtp = class {
  constructor(main, trace) {
    const self = this;
    self.trace = trace;

    self.main = main;

    self.Smtp = require('nodemailer');

    self.onces = {};
    self.ons = {};

    self._trace(`Constructed: ${exports.stringify(self, null, 4)}`);
  }

  _error(err) {
    const self = this;
    self._trace(err);
    console.error(err);
  }

  _trace(what) {
    const self = this;
    if (!self.trace)
      return;
    what = exports.nx.CloneObj(what);
    const stack = new Error().stack.split('\n');
    stack[0] = what;
    self.trace.push(stack);
  }

  do_event(ev) {
    const self = this;
    const args = Array.from(arguments).slice(1)[0];

    self._trace(`Event: ${ev.toString()}: args: ${args}`);

    let cb = self.onces[ev];
    if (cb)
      delete self.onces[ev];
    else
      cb = self.ons[ev];

    if (cb)
      return cb(args);
  }


  once(ev, cb) {
    const self = this;
    self.onces[ev] = cb;
    self._trace(`Once event ${ev.toString()} armed`);
  }

  on(ev, cb) {
    const self = this;
    self.ons[ev] = cb;
    self._trace(`On event ${ev.toString()} armed`);
  }

  arm_events(what, value) {
    const self = this;

    what.on('end', () => {
      self.do_event('end', value);
      delete self.smtp;
    });

    what.on('alert', () => {
      self.do_event('alert', value);
    });

    what.on('close', () => {
      self.do_event('close', value);
    });
  }

  end() {
    const self = this;
    if (self.smtp)
      self.smtp.end();
  }

  connect(options) {
    const self = this;
    self.options = options;

    self._trace(`Connect: ${exports.stringify(options, null, 4)}`);

    const config = {
      host: options.host.name,
      port: options.host.port,
      secure: false,
      auth: {
        user: options.auth.user,
        pass: options.auth.password
      }
    };

    if (options.host.tls)
      config.tls = {ciphers: 'SSLv3'};

    self.smtp = self.Smtp.createTransport(config);
  }


  send(options, text) {
    const self = this;

    options = exports.nx.CloneObj(options);
    options.text = text;

    self._trace(`Send: ${exports.stringify(options, null, 4)}`);

    self._trace(`Send promising`);

    return new Promise(function (resolve, reject) {
      try {
        self.smtp.sendMail(options, function (error, info) {
          if (error) {
            self._error(`Send ${options} failed with err=${exports.stringify(err)}`);
            reject(err);
          } else {
            resolve(info.response);
          }
        });
      } catch (err) {
        reject(err);
      }
    });
  }
}


exports.Imap = class {
  constructor(main, trace) {
    const self = this;
    self.trace = trace;

    self.main = main;

    self.files = {};
    self.Imap = require('imap');

    self.onces = {};
    self.ons = {};

    self._trace(`Constructed: ${exports.stringify(self, null, 4)}`);
  }

  _error(err) {
    const self = this;
    self._trace(err);
    console.error(err);
  }

  _trace(what) {
    const self = this;
    if (!self.trace)
      return;
    what = exports.nx.CloneObj(what);
    const stack = new Error().stack.split('\n');
    stack[0] = what;
    self.trace.push(stack);
  }

  do_event(ev) {
    const self = this;
    const args = Array.from(arguments).slice(1)[0];

    self._trace(`Event: ${ev.toString()}: args: ${args}`);

    let cb = self.onces[ev];
    if (cb)
      delete self.onces[ev];
    else
      cb = self.ons[ev];

    if (cb)
      return cb(args);
  }

  once(ev, cb) {
    const self = this;
    self.onces[ev] = cb;
    self._trace(`Once event ${ev.toString()} armed`);
  }

  on(ev, cb) {
    const self = this;
    self.ons[ev] = cb;
    self._trace(`On event ${ev.toString()} armed`);
  }

  arm_events(what, value) {
    const self = this;

    what.on('end', () => {
      self.do_event('end', value);
      delete self.imap;
    });

    what.on('alert', () => {
      self.do_event('alert', value);
    });

    what.on('mail', () => {
      self.do_event('mail', value);
    });

    what.on('expunge', () => {
      self.do_event('expunge', value);
    });

    what.on('uidvalidity', () => {
      self.do_event('uidvalidity', value);
    });

    what.on('update', () => {
      self.do_event('update', value);
    });

    what.on('close', () => {
      self.do_event('close', value);
    });
  }

  end() {
    const self = this;
    if (self.imap)
      self.imap.end();
  }

  connect(options) {
    const self = this;

    self._trace(`Connect: ${exports.stringify(options, null, 4)}`);

    self._trace(`Connect promising`);

    return new Promise(function (resolve, reject) {
      try {

// Implementation
        const config = {
          user: options.auth.user,
          password: options.auth.password,
          port: options.host.port,
          tls: options.host.tls,
          host: options.host.name
        };

        self.imap = new self.Imap(config);
        self._trace(`Connect Imap`);
        self.arm_events(self.imap, options);

        self.imap.once('ready', () => {
          self.do_event('ready', options);
          resolve(options);
        });
        self._trace(`Connect once.ready event armed`);

        self.imap.once('error', (err) => {
          self.do_event('error', err, options);
          self._error(`Failed to connect err=${exports.stringify(err)}`);
          reject(err, options);
          self.imap.end();
          delete self.imap;
        });
        self._trace(`Connect once.error event armed`);

        self._trace(`Connect starting connect`);
        self.imap.connect();
      } catch (err) {
        reject(err);
      }
    });
  }


  open(options) {
    const self = this;

    self._trace(`Open: ${exports.stringify(options, null, 4)}`);

    self._trace(`Open promising`);

    return new Promise(function (resolve, reject) {
      try {

// Implementation

        self._trace(`Starting openBox`);
        self.imap.openBox(options.box, options.readonly, (err, box) => {
          if (err) {
            self._error(`Failed to openBox err=${exports.stringify(err)}`);
            reject(err);
          }

          self._trace(`Box ${options.box} open`);
          resolve(options);
        });
      } catch (err) {
        reject(err);
      }
    });
  }


  search(options) {
    const self = this;

    self._trace(`Search: ${exports.stringify(options, null, 4)}`);

    self._trace(`Search promising`);

    return new Promise(function (resolve, reject) {
      try {
        self.imap.search(options.search, (err, messages) => {
          if (err) {
            self._error(`Search ${options.search} failed with err=${exports.stringify(err)}`);
            reject(err);
          }

          self._trace(`Search ${options.search} completed with ${messages.length} messages`);
          resolve(messages);
        });
      } catch (err) {
        reject(err);
      }
    });
  }


  getMsg(msg, seq) {
    const self = this;

    self._trace(`getMsg: ${seq}`);

    self._trace(`getMsg promising`);

    return new Promise(function (resolve, reject) {
      try {
        let buffer = '';

        msg.on('body', (stream, info) => {
          self._trace(`Message body seq=${seq}`);

          stream.on('data', (chunk) => {
            buffer += chunk.toString();
          });
          self._trace(`Stream on.data event message seq=${seq} armed`);

          stream.once('end', () => {
            self._trace(`Message end: seq=${seq}`);
            // Mark the above mails as read
            msg.once('attributes', function (attrs) {
              let uid = attrs.uid;
              self.imap.addFlags(uid, ['\\Seen'], function (err) {
                if (err) {
                  console.log(err);
                } else {
                  console.log("Marked as read!")
                }
              });
            });
            resolve(buffer);
          });
          self._trace(`Message once.end event message seq=${seq} armed`);
        });
        self._trace(`Message on.body event message seq=${seq} armed`);
      } catch (err) {
        reject(err);
      }
    });
  }


  fetch(options) {
    const self = this;

    self._trace(`Fetch: ${exports.stringify(options, null, 4)}`);

    self._trace(`Fetch promising`);

    return new Promise(function (resolve, reject) {
      const emails = [];

// Implementation
      try {
        self._trace(`Fetch Starting`);
        self._fetch = self.imap.fetch(options.messages, {bodies: ''});
        self._trace(`Fetch started`);

        self._fetch.on('message', (msg, seq) => {
          self._trace(`Fetch ready for message seq=${seq}`);

          self.getMsg(msg, seq).catch((err) => {
            reject(err);
          }).then((buffer) => {
            self._trace(`Message end: seq=${seq}`);
            emails.push({seq: seq, body: buffer});
            if (emails.length === options.messages.length)
              resolve(emails);
          });
        });
        self._trace(`Fetch on.message event armed`);

        self._fetch.once('error', (err) => {
          self._error(`Fetch err=${exports.stringify(err)}`);
          reject(err);
        });
        self._trace(`Fetch once.error event armed`);

        self._fetch.once('end', () => {
          self._trace(`Fetch end`);
        });
        self._trace(`Fetch once.end event armed`);
      } catch (err) {
        if (err.message === 'Nothing to fetch')
          return resolve(emails);
        reject(err);
      }
    });
  }
}

exports.Imap.parse = function parse(email) {
  const self = this;

  const content = [];
  const errors = [];
  let headers = [];
  let unknowns = [];
  let bidValue;


  function isHeader(line) {
    let j, ch;
    for (j = 0; j < line.length; ++j) {
      ch = line.charAt(j).toLowerCase();
      if (!((ch === '-') || (ch === '_') || (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z')))
        break;
    }
    if (j < line.length && ch === ':')
      return {name: line.slice(0, j), value: line.slice(j + 1).trim()};
    return undefined;
  }


  function getBidValue(header) {
    try {
      let bid = header.value;
      if (exports.nx.isArray(bid))
        bid = bid.join(' ');
      let idx = bid.toLowerCase().indexOf('boundary=');
      if (idx > 0)
        bidValue = exports.nx.replaceAll(bid.slice(idx + 10), ['=', '"'], ['', '']);
    } catch (err) {
      errors.push(`${exports.stringify(header)}\n${err.toString()}`);
    }
  }


  function collectHeader(lines, i) {
    for (i = i + 1; i < lines.length; ++i) {
      line = lines[i];
      if (line.length === 0)
        break;      // going into content...

      const h = isHeader(line);
      if (h || (h && h.name !== header.name)) {
        if (header.name.toLowerCase().trim() === 'content-type')
          getBidValue(header);
        headers.push(header);
        if ((header = h))
          return collectHeader(lines, i);
      }
      if (!exports.nx.isArray(header.value))
        header.value = [header.value];
      header.value.push(line);
    }

    return i;
  }


  let header, line, i, j;
  let body = exports.nx.CloneObj(email.body);
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
    if (header.name.toLowerCase().trim() === 'content-type')
      getBidValue(header);
    headers.push(header);
    header = undefined;
  }

  let hdrs = exports.nx.CloneObj(headers);
  let unks = exports.nx.CloneObj(unknowns);
  headers = [];
  unknowns = [];
  let bid_line;
  for (i = i + 1; i < lines.length; ++i) {
    line = lines[i];
    if (line.length === 0)
      continue;   // skip

    if (bidValue && line.indexOf(`=${bidValue}=`) >= 0) {
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
    if (header.name.toLowerCase().trim() === 'content-type')
      getBidValue(header);
    headers.push(header);
    header = undefined;
  }

  content.push({headers: headers, unknowns: unknowns});
  headers = hdrs;
  unknowns = unks;
  return {
    seq: email.seq,
    boundary_id: bidValue,
    headers: headers,
    content: content,
    unknowns: unknowns,
    errors: errors,
    body: email.body
  };
}
