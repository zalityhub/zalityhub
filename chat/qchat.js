exports.Qchat = class {

  constructor(main, trace) {
    const self = this;
    self.trace = trace;

    self.main = main;

    self.stringify = require('json-stringify-safe');
    self.nx = require('../util/util.js');

    self.onces = {};
    self.ons = {};

    self.env = self.nx.getEnv(['chatgpt'], true);
    self.chatConfig = self.nx.CloneObj(self.env.chat_service);
    self.davinci = self.chatConfig.settings.model_methods.items.davinci;
    self.config = self.davinci.config;
    self.dialog = self.davinci.dialog;

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

  parseGptResponse(gptResponse) {
    const self = this;
    const response = {json: gptResponse};

    // extract the response text
    let json = gptResponse;
    try {
      let text = '';
      if (json.error) {
        json = json.error;
        if (json.type && json.message)
          json = `${json.type.toString()}: ${json.message.toString()}`;
        else if (json.message)
          json = json.message.toString();
      }
      if (json.choices)
        json = json.choices;
      if (self.nx.isArray(json)) {
        for (let i = 0, ilen = json.length; i < ilen; ++i)
          if (json[i].text)
            text += `${json[i].text}\n`;
          else if (json[i].message)
            text += `${json[i].message.content}\n`;
      } else
        text = json.toString();
      response.text = text.toString().trim();
    } catch (err) {
      response.text = self.stringify(gptResponse);
    }

    return response;
  }


  sendquery(question) {
    const self = this;

    delete self.dialog.system;
    delete self.dialog.user;

    self.dialog.prompt = `assistant: You are a chatbot\nuser: ${question}\n`;

    return new Promise(function (resolve, reject) {
      try {

        // get the http request

        self.config.headers.Authorization = self.chatConfig.settings.model_methods.Authorization;

        const protocol = require(self.config.type);  // http or https
        const req = protocol.request(self.config, (res) => {
          res.setEncoding('utf8');
          let body = '';

          res.on('data', (chunk) => {
            body += chunk;
          });

          res.on('end', () => {
            delete self.config.headers.Authorization;
            resolve(self.parseGptResponse(JSON.parse(body.toString())));
          });

          req.on('error', (err) => {
            reject(err);
          });
        });

// send the query
        self.dialog.max_tokens = 3900;
        req.write(self.stringify(self.dialog));
        req.end();
      } catch (err) {
        reject(err);
      }
    });
  }


  Query(argv) {
    const self = this;

    return new Promise(function (resolve, reject) {
      try {
        self.sendquery(argv).then((result) => {
          resolve(result);
        }).catch((err) => {
          reject(err);
        });
      } catch (err) {
        reject(err);
      }
    });
  }
}
