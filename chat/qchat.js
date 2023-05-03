const stringify = require('json-stringify-safe');

const nx = require('../util/util.js');
const Env = nx.getEnv(['chatgpt'], true);
const ChatConfig = nx.CloneObj(Env.chat_service);
const davinci = ChatConfig.settings.model_methods.items.davinci;
const Config = davinci.config;
const Dialog = davinci.dialog;


function ParseGptResponse(gptResponse) {
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
    if (nx.isArray(json)) {
      for (let i = 0, ilen = json.length; i < ilen; ++i)
        if (json[i].text)
          text += `${json[i].text}\n`;
        else if (json[i].message)
          text += `${json[i].message.content}\n`;
    } else
      text = json.toString();
    response.text = text.toString().trim();
  } catch (err) {
    response.text = stringify(gptResponse);
  }

  return response;
}


function sendquery(question, cb) {

  function requester(cb) {
    Config.headers.Authorization = ChatConfig.settings.model_methods.Authorization;
    const protocol = require(Config.type);  // http or https
    const req = protocol.request(Config, (res) => {
      res.setEncoding('utf8');
      let body = '';

      res.on('data', (chunk) => {
        body += chunk;
      });

      res.on('end', () => {
        delete Config.headers.Authorization;
        cb(null, ParseGptResponse(JSON.parse(body.toString())));
      });
    });

    req.on('error', (err) => {
      return cb(err);
    });

    return req;
  }


  delete Dialog.system;
  delete Dialog.user;

  Dialog.prompt = `assistant: You are a chatbot\nuser: ${question}\n`;

  try {

    // get the http request
    const req = requester(cb);

// then send it
    Dialog.max_tokens = 3900;
    req.write(stringify(Dialog));
    req.end();
  } catch (err) {
    cb(err);
  }
}


function Query(argv, cb) {

  if (!cb)
    cb = function (err, result) {
      if (err) return console.error(err.toString());
      console.log(result.text);
    };

  while (argv.length) {
    const q = argv.shift();

    sendquery(q, cb);
  }
}

Query(process.argv.slice(2));
