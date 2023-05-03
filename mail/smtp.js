exports.smtp = class {

  constructor(main, trace) {
    const self = this;
    self.trace = trace;

    self.main = main;

    self.fs = require('fs');
    self.stringify = require('json-stringify-safe');
    self.nx = require('../util/util.js');
    self.Smtp = require('nodemailer');

    self.onces = {};
    self.ons = {};

    self._trace(`Constructed: ${self.stringify(self, null, 4)}`);
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
    what = self.nx.CloneObj(what);
    const stack = new Error().stack.split('\n');
    stack[0] = what;
    self.trace.push(stack);
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

  end() {
    const self = this;
    if (self.smtp)
      self.smtp.end();
  }

  connect(options) {
    const self = this;
    self.options = options;

    self._trace(`Connect: ${self.stringify(options, null, 4)}`);

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

    options = self.nx.CloneObj(options);
    options.text = text;

    self._trace(`Send: ${self.stringify(options, null, 4)}`);

    self._trace(`Send promising`);
    return new Promise(function (resolve, reject) {

      self.smtp.sendMail(options, function (error, info) {
        if (error) {
          self._error(`Send ${options} failed with err=${self.stringify(err)}`);
          reject(err);
        } else {
          resolve(info.response);
        }
      });
    });
  }
}
