const fs = require('fs');
const readline = require('readline');
const stringify = require('json-stringify-safe');
const linewrap = require('linewrap');
const uuid = require('uuid');

const nx = require('../util/util.js');
const Config = nx.getEnv('chatgpt', true);
const ChatConfig = nx.CloneObj(Config.chat);
const Merge = require('lodash.merge');
const Pluralize = require('pluralize')
const String_Cosine = require('./string_cosine.js');
const Corpus = require('./Corpus.js');


function WrapWords(text, options) {

  if (!options)
    options = {};

  options = nx.CloneObj(options);

  if (!options.lineBreak)
    options.lineBreak = '\n';

  const cols = options.cols;
  if (cols) {
    delete options.cols;
    let wrap = linewrap(cols, options);
    const lch = text.charAt(text.length - 1);
    wrap = wrap(text.toString());
    if (wrap.charAt(wrap.length - 1) !== lch)  // last ch was removed
      wrap = `${wrap}${lch}`;         // add it back
    text = wrap;
  }

  return text;
}


function Pipe(pipes, text) {
  if (pipes) {
    text = text.toString();
    for (const [key, value] of Object.entries(pipes))
      value(text);    // call the pipe function
  }
}

function WriteStream(handle, text) {

  if (!handle.enabled)
    return;       // nothing to do

  let file;
  if (!(file = Config.files[handle.file])) {
    file = {stream: fs.createWriteStream(handle.file, handle.options)};
    file.handle = nx.CloneObj(handle);
    Config.files[handle.file] = file;
    WriteStream(handle, `\nOpened: ${nx.date()}\n`);
  }

  file.stream.write(text.toString());
  Pipe(handle.pipes, text);
}


function ChatLog(text) {
  WriteStream(Config.context.options.log, text);
}

function LogWrite(text, options) {

  text = text.toString();
  if (!text.length)
    return;

  if (!options)
    options = {};

  options = nx.CloneObj(options);
  if (!options.fd)
    options.fd = process.stdout.fd;

  if (options.wrap)
    fs.writeSync(options.fd, WrapWords(text, options.wrap));
  else
    fs.writeSync(options.fd, text);

  ChatLog(text);
  return text;
}

function Log(text, options) {
  if (!options)
    options = {};
  return LogWrite(`${text}\n`, options);
}

function LogError(text, options) {
  if (!options)
    options = {};
  return LogWrite(`${text}\n`, {...options, ...{fd: process.stderr.fd}});
}

function Debug(context, handle, text) {

  if (nx.isString(handle) &&
    (!(handle = nx.resolveProperty(context, `options.debug.${handle}`))))
    return false;
  if (!handle.enabled)
    return false;
  if (!text)
    return true;    // is enabled

  if(nx.isArray(text)) {
    const sb = new nx.StringBuilder();
    for(let i = 0; i < text.length; ++i )
      sb.appendLine(text[i]);
    return Debug(context, handle, sb.toString());
  }

  if (!Object.entries(handle.pipes).length && context.options.log.copydebug)    // not pipeing & copy debug enabled
    Log(text);    // repeat to console
  if (!Object.entries(handle.pipes).length && context.options.log.copydebug)    // not pipeing & copy debug enabled
    Log(text);    // repeat to console

  WriteStream(handle, `${text}\n`);

  Pipe(context.options.debug.pipes, text);
  return text;
}

function ElementString(name, el, indent) {
  indent = indent ? indent : '';
  const sb = new nx.StringBuilder();

  if (nx.isNull(el)) {
    return `${indent + name}(null): ${el}`;
  }
  if (nx.isObject(el)) {
    sb.appendLine(indent + name + ' {');
    const keys = Object.keys(el);
    for (let i = 0, ilen = keys.length; i < ilen; ++i) {
      const key = keys[i];
      const text = ElementString(key, el[key], indent + '  ');
      sb.append(text);
    }
    sb.appendLine(indent + '}');
    return sb.toString();
  }
  if (nx.isArray(el)) {
    sb.appendLine(indent + name + ' [');
    for (let i = 0, ilen = el.length; i < ilen; ++i) {
      const key = i.toString();
      const text = ElementString(key, el[i], indent + '  ');
      sb.append(text);
    }
    sb.appendLine(indent + ']');
    return sb.toString();
  }
  if (nx.isStringBuilder(el)) {
    const text = ElementString(name, el.array, indent);
    sb.append(text);
    return sb.toString();
  }
  if (nx.isFunction(el)) {
    sb.append(el.toString());
    return sb.toString();
  }
  const typ = typeof el;
  sb.appendLine(`${indent + name}(${typ}): "${el.toString()}"`);
  return sb.toString();
}


function BuildContext(context) {

  context = context ? context : {};
  Config.context = context;     // current context in use

  // check the given context, merge default for any missing properties...
  // get a list of passed properties
  const config_keys = Object.keys(ChatConfig);
  const properties = [];
  for (let i = 0, ilen = config_keys.length; i < ilen; ++i)
    properties.push(context[config_keys[i]]);

// merge passed context with the defaults from ChatConfig
  for (let i = 0, ilen = config_keys.length; i < ilen; ++i) {
    const key = config_keys[i];

    if (nx.isArray(ChatConfig[key])) {
      if (properties[i])    // if we had a previous array
        context[key] = ChatConfig[key].concat(properties[i]); // add it to the end
      else
        context[key] = [].concat(ChatConfig[key]);
    } else if (nx.isObject(ChatConfig[key])) {
      context[key] = nx.CloneObj(ChatConfig[key]);
      if (properties[i])    // if passed an object
        Merge(context[key], properties[i]);
    } else if (nx.isString(ChatConfig[key])) {
      if (!properties[i])    // if not passed a string
        context[key] = (' ' + ChatConfig[key]).slice(1);    // force a copy
    }
  }

// select and verify

  if (!context.options) {
    context.options = nx.CloneObj(context.settings);
    context.options.uuid = uuid.v4();
    context.options.etc = {};
    context.options.etc.session = {date: `${nx.date()}`, log: [], errors: []};

    // set up debug/log pipes
    Object.keys(context.options.debug).forEach(function (key) {
      context.options.debug[key].pipes = {};
    });
    context.options.log.pipes = {};
    context.options.debug.pipes = {};

// select the model
    let model;
    if (nx.isArray(context.options.model))
      model = context.options.model[0];    // use the first one
    else
      model = context.options.model;
    context.model = context.models[model];
    if (!context.model) {
      Log(`Model ${model} is not available`);
      const models = Object.entries(context.models)[0];
      mode = context.options.model = models[0];
      context.model = models[1];
    }
    Log(`Using model ${model}`);
  }

  const model = context.model;

// convert any strings to numerics...
  if (nx.isString(model.dialog.temperature))
    model.dialog.temperature = parseFloat(model.dialog.temperature);
  if (nx.isString(model.dialog.max_tokens))
    model.dialog.max_tokens = parseInt(model.dialog.max_tokens);
  if (nx.isString(model.dialog.top_p))
    model.dialog.top_p = parseFloat(model.dialog.top_p);
  if (nx.isString(model.dialog.frequency_penalty))
    model.dialog.frequency_penalty = parseFloat(model.dialog.frequency_penalty);
  if (nx.isString(model.dialog.presence_penalty))
    model.dialog.presence_penalty = parseFloat(model.dialog.presence_penalty);

  Debug(context, 'context', ElementString('context', context));
  return context;
}


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


function SortHistory(history) {
  return history.sort(function (a, b) {     // sort by ratio
    return (a.ratio ? a.ratio : 0) - (b.ratio ? b.ratio : 0);
  });
}


function PruneHistory(history, ratio, cb) {

  /*
      {
          "uuid": "9362eee0-fe7f-4274-a802-9cfc70b8b054",
          "date": "2023-04-07 01:58:01.701Z",
          "user": "who discovered pinellas county",
          "bot": "Pinellas County..."
          "method": "tf-idf",
          "role": "bot",
          "ratio": 1.7621149852136642,
          "hit": "wheres are las jollas",
          "on": "Pinellas County ..."
      }
  */


  const nodups = [];
  const seen = new Set();
  for (let h of history) {
    const uuid = h.uuid;
    if (!seen.has(uuid)) {
      seen.add(uuid);
      if (!ratio || h.ratio >= ratio)
        nodups.push(nx.CloneObj(h));
      else if (cb)
        cb(h, ratio);   // show discards
    }
  }

  return SortHistory(nodups);
}


function GetSimilar(similar, question, history) {
  /*
      history: [{
          "uuid": "bf1ff057-036d-401b-a070-4736976a8bce",
          "user": "what is the sqrt of -12167",
          "bot": " The square root of -12167 is not a real number."
      }]
  */

  history = nx.CloneObj(history);

  const map = {};
  const h1 = [];
  const h2 = [];
  for (let i = 0; i < history.length; ++i) {
    const h = nx.CloneObj(history[i]);
    map[h.uuid] = h;

    if (h.user) {
      h1.push(`user.${h.uuid}`);
      h2.push(h.user.trim().toLowerCase());
    }
    if (h.bot) {
      h1.push(`bot.${h.uuid}`);
      h2.push(h.bot.trim().toLowerCase());
    }
  }
  const corpus = new Corpus.Corpus(h1, h2);

  function pluralize(text) {
    text = text.split(' ');   // words
    for (let i = 0; i < text.length; ++i)
      text[i] = Pluralize(text[i]);
    return text.join(' ');
  }

  function singularize(text) {
    text = text.split(' ');   // words
    for (let i = 0; i < text.length; ++i)
      text[i] = Pluralize.singular(text[i]);
    return text.join(' ');
  }

  function getHits(question) {
    const results = [];
    const hits = corpus.getResultsForQuery(question);
    for (let i = 0; i < hits.length; ++i) {
      const ratio = hits[i][1];
      let uid = hits[i][0];
      const d = uid.indexOf('.');
      const role = uid.slice(0, d);
      uid = uid.slice(d + 1);
      const h = nx.CloneObj(map[uid]);
      h.method = method;
      h.role = role;
      h.ratio = ratio;
      h.hit = question;
      h.on = h[role];
      results.push(h);
    }
    return results;
  }

  similar = nx.CloneObj(similar);
  const method = similar.method.shift();
  let results = [];
  results = results.concat(getHits(question));
  results = results.concat(getHits(singularize(question)));
  results = results.concat(getHits(pluralize(question)));
  return PruneHistory(results);
}


function SelectHistory(context, question) {
  const similar = nx.CloneObj(context.options.similar);

  question = question.trim().toLowerCase();

  let history = [];

  if (context.options.enabled.history) {
    const last = [].concat(context.history.slice(0 - similar.minimum_history)).map(v => {
      return {...v, ...{method: 'last', ratio: 99999}};    // get last N
    });

    history = GetSimilar(similar, question, context.history);
    history = PruneHistory(history.concat(last));

    Debug(context, 'history', [
      'selected_history:',
      '  ----------------------------------------------',
      `  ${stringify(history, null, 4)}`,
      '  ----------------------------------------------',
      `${history.length} items`
    ]);
  }

  return nx.CloneObj(history);
}


function SaveHistory(context, question, response) {

  const entry =
    {
      uuid: uuid.v4(),
      date: `${nx.date()}`,
      user: nx.replaceAll(question.trim(), ['assistant:', 'bot:'], ['', ''], true),
      bot: nx.replaceAll(response.trim(), ['assistant:', 'bot:'], ['', ''], true)
    };

  context.history.push(entry);
}


function SendChatReq(context, question, callback) {

  function logSession(session) {
    if (Debug(context, 'session')) {
      const dsession = nx.CloneObj(session);
      if (nx.resolveProperty(dsession, 'model.history'))
        delete dsession.model.history;
      return Debug(context, 'session', `session=${stringify(dsession, null, 4)}`);
    }
    return '';
  }

  function sDebug(handle, text) {
    if (Debug(context, handle))
      session.log.push(Debug(context, handle, text));
    return text;
  }

  function sLog(text) {
    session.log.push(Log(text));
    return text;
  }

  function sLogError(text) {
    session.errors.push(LogError(text));
    return text;
  }

  function callbacker(err, rsp) {
    if (!callback)
      callback = function () {
      };

    if (err)
      sLogError(err.toString());

    logSession(session);
    if (rsp)
      rsp.session = session;
    callback(err, rsp);
  }


  function requester(cb) {
    const protocol = require(model.config.type);  // select http or https
    const req = protocol.request(model.config, (res) => {
      res.setEncoding('utf8');
      let body = '';

      res.on('data', (chunk) => {
        body += chunk;
      });

      res.on('end', () => {
        cb(null, body.toString());
      });
    });

    req.on('error', (err) => {
      return cb(err);
    });

    return req;
  }

  function buildDialog() {

// Make a copy of the dialog and history object
    session.dialog = nx.CloneObj(model.dialog);
    const system = session.dialog.system;
    const user = session.dialog.user
    delete session.dialog.system;
    const history = nx.CloneObj(session.history);

    session.prompt = null;
    session.word_count = 0;
    session.token_count = 0;

    switch (session.dialog.model) {
      default:
        session.prompt = [];
        while (history.length) {
          const h = nx.CloneObj(history.shift());
          if (h.user) {
            session.prompt.push({role: user, content: h.user});
            session.word_count += h.user.split(' ').length;
          }
          if (h.bot) {
            // h.bot = nx.remove_stopwords(h.bot);
            session.prompt.push({role: system, content: h.bot});
            session.word_count += h.bot.split(' ').length;
          }
        }
        session.dialog.messages = session.prompt;    // pass the prompt
        session.token_count = Math.floor(session.word_count * model.token_ratio);
        sDebug('logic', `Using a token_ratio of ${model.token_ratio}, ${session.word_count} words makes ${session.token_count} tokens`);
        if ((session.dialog.max_tokens -= session.token_count) < 100)
          session.dialog.error = `model ${session.dialog.model} max_token ${session.dialog.max_tokens} too small`;
        break;

      case 'text-davinci-003':
        session.prompt = new nx.StringBuilder();
        while (history.length) {
          const h = nx.CloneObj(history.shift());
          if (h.user) {
            session.prompt.append(`${user}: ${h.user}\n`);
            session.word_count += h.user.split(' ').length;
          }
          if (h.bot) {
            session.prompt.append(`${system}: ${h.bot}\n`);
            session.word_count += h.bot.split(' ').length;
          }
        }
        session.dialog.prompt = session.prompt.toString();
        session.word_count = session.dialog.prompt.split(' ').length;
        session.token_count = Math.floor(session.word_count * model.token_ratio);
        sDebug('logic', `Using a token_ratio of ${model.token_ratio}, ${session.word_count} words makes ${session.token_count} tokens`);
        if ((session.dialog.max_tokens -= session.token_count) < 100)
          session.dialog.error = `model ${session.dialog.model} max_token ${session.dialog.max_tokens} too small`;
        break;
    }
    return session.dialog;
  }

  function buildReq() {
    model.config.headers.Authorization = ChatConfig.settings.Authorization;
    model.history = SelectHistory(context, context.question);
    session.history = nx.CloneObj(model.history);

    session.history.push({uuid: uuid.v4(), user: context.question});    // add this question

      const dialog = buildDialog();
      if (dialog.error) 
        return '';

    session.reqData = stringify(dialog);
    return session.reqData;
  }

  function handleError(err) {
    return callbacker(err);

    sLog(err);
    if (err.indexOf('maximum context length is') > 0 ||
      err.indexOf('is less than the minimum of') > 0) {    // hit max tokens

      const words = err.split(' ');

      /*
      This model's maximum context length is 4096 tokens. However, you requested 4221 tokens (1829 in the messages, 2392 in the completion). Please reduce the length of the messages or completion.
      */

      model.token_ratio *= 1.1;
      context.options.similar.min_ratio *= 1.1;
      sLog(`Retrying to answer '${context.question}' with token ratio of ${model.token_ratio}`);
      logSession(session);
      session = {date: `${nx.date()}`, log: [], errors: []};    // reset
      context.options.etc.session = session;
      return true;      // say retry
    } else if (err.indexOf(' max_token ') > 0) {
    }

    return false;     // no retry
  }

  function updateRatios() {
    const usage = nx.resolveProperty(context, 'response.json.usage');
    if (!usage)
      return;   // none

    const token_ratio = usage.prompt_tokens / session.word_count;
    if (token_ratio < model.token_ratio)
      return;   // never regress

    let delta = token_ratio / model.token_ratio;
    if (delta > 1.0)
      delta = 1.0 / delta;
    delta = Math.floor((1.0 - delta) * 100.0);
    const msg = `Used token_ratio: ${model.token_ratio}\n` +
      `Reported token_ratio: ${token_ratio}\n` +
      `Delta diff: ${delta}%`;
    sDebug('logic', msg);
    if (delta > 10) {    // update the model
      sLog(msg + '\nUpdating the model');
      model.token_ratio = token_ratio;
    }
  }

  function receiver(err, body) {
    if (nx.resolveProperty(model, 'config.headers.Authorization'))
      delete model.config.headers.Authorization;    // remove the key

    if (err)
      return handleError(err);

    session.body = nx.replaceAll(body.toString().trim(), ['assistant:', 'bot:'], ['', ''], true);
    context.response = ParseGptResponse(JSON.parse(session.body));
    session.response = context.response;

    if (nx.resolveProperty(context, 'response.json.error.message'))
      return handleError(context.response.json.error.message);

    ChatLog(`Response: [${stringify(context.response.text)}]\n`);
    SaveHistory(context, context.question, context.response.text);

    updateRatios();

    return callbacker(null, context);
  }


  context = BuildContext(context);

  context.options.etc.session = {date: `${nx.date()}`, log: [], errors: []};
  let session = context.options.etc.session;

  const model = context.model;
  session.model = model;

  try {
    question = (!question) ? '' : question.toString().trim();
    if (!question.length) {
      const err = new Error('no question given');
      return callbacker(err);
    }

    context.question = question;
    ChatLog(`Question: [${context.question}]\n`);   // put in the log

    sDebug('logic', `Using model ${model.dialog.model}`);


    // build request data
    const reqData = buildReq();
    if (!reqData.length) {  // could not build
      sLogError(session.dialog.error);
      if( session.dialog.error.search(/max_token.*too small/g) > 0)
        sLogError(`${stringify(session.history, null, 4)}\nIs too much history`);
      return callbacker(session.dialog.error);
    }

    // get the session
    const req = requester(receiver);

// then send it
    req.write(reqData);
    req.end();
  } catch (err) {
    callbacker(new Error(`${err.toString()}\n${err.stack}`));
  }
}


function SummarizeConversation(context, cb) {
  const ctx = nx.CloneObj(context);
  ctx.options.debug.history.enabled = false;
  ctx.options.debug.session.enabled = false;
  ctx.options.debug.context.enabled = false
  ctx.options.debug.logic.enabled = false
  ctx.options.enabled.history = true;

  SendChatReq(ctx, 'summarize our conversation', cb, true);
}


function SelectSimilarity(text, response) {
  const ctx = nx.CloneObj(context);
  ctx.options.enabled.history = true;
  ctx.options.debug.logic.enabled = true;
  ctx.options.debug.logic.pipes[arguments.callee.name] = function (text) {
    if (response)
      response.appendLine(text);
  };

  const history = SelectHistory(ctx, text);

  const summary = `${history.length} history items selected`;
  if (response)
    return response.appendLine(summary);
  return summary;
}


function Ask(context, questions, callback, options) {

  function ask(q, cb) {
    if (q.length) {
      if (options.echo)
        sb.appendLine(`\n${q}\n`);
      if (context.options.enabled.commands && q.indexOf('.') === 0)
        return ChatCommand(context, q.slice(1).trim(), cb);
      return SendChatReq(context, q, cb);
    }
    nextAsk();    // try next
  }

  function nextAsk() {
    if (!questions.length)
      return callback(null, sb.toString());

    const q = questions.shift();
    ask(q.trim(),
      function (err, result) {
        if (err) {
          err = `\n${err.toString()}\n`;
          sb.appendLine(err);
          return callback(err, sb.toString());
        }

        if (nx.resolveProperty(result, 'response.text'))
          sb.appendLine(`\n${result.response.text}\n`);
        else
          sb.appendLine(`\n${result.toString()}\n`);

        nextAsk();    // send next
      });
  }

  if (!options)
    options = {echo: true};

  callback = callback ? callback : function (err, result) {
    if (err)
      return Log(err, options);

    LogWrite('\n');
    Log(result, options);
    LogWrite('\n');
  };

  if (!nx.isArray(questions))
    questions = [questions];
  else
    questions = [].concat(questions);

  const sb = new nx.StringBuilder();

  nextAsk();
}


function StartWebService(argv) {

  const express = require('express');
  const cors = require('cors');
  const Url = require('url');

  argv = argv.join('');
  if (argv.length <= 0)
    argv = Config.proxy.proxyServerUrl;   // no arg, use the configured url

  const url = Url.parse(argv);

  const app = express();
  app.use(express.json());
  app.use(cors())
  app.get(`${url.path}*`, (req, res) => {
    try {
      let question = req.url.slice(url.path.length);
      if (question.charAt(0) === '?' || question.charAt(0) === '&')
        question = question.slice(Math.max(1, question.indexOf('=') + 1));

// A Get request never supplies context
      Ask(BuildContext(), question, function (err, result) {
        res.send(result.response.text);
      });
    } catch (err) {
      res.end('Internal error');
    }
  });

  app.post(`${url.path}*`, (req, res) => {
    // Get the prompt from the request
    let body = '';
    req.on('data', function (chunk) {
      body += chunk;
    });

    req.on('end', function () {
      body = body.toString().trim();
      if (!body.length)
        return res.end();
      const context = BuildContext(JSON.parse(body));
      Ask(context, context.question, function (err, result) {
        if (nx.isStringBuilder(result) || nx.isString(result)) {
          result = result.toString('');
          context.response = {text: result.toString()};
          context.response.json = stringify(context.response.text);
          result = context;
        }
        res.send(stringify(result));
      });
    });
  });

  app.listen(url.port, url.hostname, () => {
    Log(`Server listening on ${argv}`);
  });
}


function FindHits(args, obj, typ, prior) {

  function getCmdString(result) {
    const sb = new nx.StringBuilder();
    while (result && result.length === 1) {
      sb.insert(0, result.hits[0].key);
      result = result.prior;
    }
    return sb.toString({post: ' '}).trim();
  }


  function buildResult(args, obj, typ, prior) {
    let arg = args = (' ' + args).slice(1).trim();    // force a copy
    const i = arg.indexOf(' ');   // first space
    if (i > 0)
      arg = arg.slice(0, i).trim();

    const argv = {args: args, arg: arg, tail: args.slice(arg.length + 1).trim()};
    const result = {matched: false, hits: [], length: 0, argv: argv, cmdString: ''};

    if (prior) {		// this is a recursive call
      prior = nx.CloneObj(prior);
      result.prior = prior;
      argv.oargs = prior.argv.oargs;		// copy the original arg string
    } else {
      argv.oargs = args;					// first call, save the arg string
    }

    return result;
  }


  const result = buildResult(args, obj, typ, prior);
  const argv = result.argv;
  const arg = argv.arg;
  let hits = result.hits;

  if (!arg.length)     // there is no arg
    return result;					// a complete miss

// find matching keys
  const keys = Object.keys(obj);
  for (let i = 0, ilen = keys.length; i < ilen; ++i) {
    const key = keys[i];
    if (key.indexOf(arg) === 0)
      hits.push({hit: obj, val: obj[key], key: key, what: nx.isWhat(obj[key])});
  }

  result.hits = hits;
  result.length = hits.length;
  result.cmdString = getCmdString(result);

  if (hits.length === 1) {			// a unique match
    if (hits[0].what.is(typ)) {      // and the desired type
      result.matched = hits[0];		// Bingo
    } else {							// not the type, look forward using tail
      const r = FindHits(argv.tail, hits[0].val, typ, result);
      if (r.length === 1)
        return r;			// found something...
      if (r.length) {
        hits = r.hits;		// have some hits, move it forward
        result.hits = hits;
        result.length = hits.length;
      }
    }
  }

  if (hits.length === 0) { // a miss, try splitting the command string
    // establish the initial split values
    argv.split = result.prior ? (nx.isNull(result.prior.argv.split) ? 0 : (result.prior.argv.split + 1)) : 0;
    const oargs = argv.oargs;
    for (let split = argv.split; split < oargs.length; ++split) {
      const nargs = oargs.slice(0, split + 1);
      const tail = oargs.slice(split + 1);
      const r = FindHits(`${nargs} ${tail}`, obj, typ, result);
      if (r.length === 0 || r.length === 1)
        return r;			// found something...
    }
  }

  return result;
}


function BuildHelp(item, bopts) {

  function doFunction(sb, fun, bopts) {
    sb.append(`${fun.name}`);
    const opts = fun({help: true});		// ask function for options
    if (opts.obj) {
      bopts.delim = '|';
      bopts.indent = '';
      sb.append(doItem(opts.obj, bopts));
    }

    bopts.delim = delim;
    bopts.indent = indent;
  }

  function doObject(sb, obj, bopts) {
    const keys = Object.keys(obj);
    for (let i = 0, ilen = keys.length; i < ilen; ++i) {
      const key = keys[i];
      const it = obj[key];
      sb.append(`${sb.length ? delim : ' '}${indent}${key}`);
      // if (!single)
      // single = key;
      if (nx.isFunction(it) || nx.isObject(it) || nx.isArray(it))
        sb.append(`${BuildHelp(it, bopts)}`);
    }
  }

  function doArray(sb, ar, bopts) {
    for (let i = 0, ilen = ar.length; i < ilen; ++i) {
      const it = ar[i];
      if (nx.isFunction(it) || nx.isObject(it) || nx.isArray(it)) {
        // if (!single)
        // single = it;
        sb.append(`${sb.length ? delim : ' '}${indent}${it}`);
        sb.append(`${BuildHelp(it, bopts)}`);
      }
    }
  }

  function doHits(sb, hits, bopts) {
    for (let i = 0, ilen = hits.length; i < ilen; ++i) {
      const hit = hits[i];
      // if (!single)
      // single = hit.key;
      sb.append(`${sb.length ? delim : ' '}${indent}${hit.key}`);
      if (hit.what.isObject)
        sb.append(`${BuildHelp(hit.val, bopts)}`);
    }
  }

  function doItem(item, bopts) {
    const sb = new nx.StringBuilder();

    if (item.hits)     // these are from hits...
      doHits(sb, item.hits, bopts);
    else if (nx.isObject(item))
      doObject(sb, item, bopts);
    else if (nx.isArray(item))
      doArray(sb, item, bopts);
    else if (nx.isFunction(item))
      doFunction(sb, item, bopts);
    return sb.toString();
  }


//
// BuildHelp(item, bopts)
//
  bopts = nx.CloneObj(bopts);
  let delim = bopts.delim ? bopts.delim : '\n';
  let indent = bopts.indent ? bopts.indent : '';
  let depth = bopts.depth ? bopts.depth : 16;

  bopts.delim = delim;
  bopts.indent = indent + '  ';
  bopts.depth = depth - 1;
  if (bopts.depth < 0)
    return '';				// done

  return doItem(item, bopts);
}


function FindMisses(obj, indent, depth) {
  indent = indent ? indent : '';

  if (depth === undefined || depth === null)
    depth = 16;			// arbitrary depth

  if (--depth < 0)
    return '';				// done

  const sb = new nx.StringBuilder();
  let first;

  if (obj.hits) {   // these are from hits...
    const hits = obj.hits;
    for (let i = 0, ilen = hits.length; i < ilen; ++i) {
      const hit = hits[i];
      if (i === 0)
        first = hit.key;
      sb.append(`\n${indent}${hit.key}`);
      if (hit.what.isObject)
        sb.append(` ${FindMisses(hit.val, indent + '  ', depth)}`);
    }
  } else {
    if (nx.isObject(obj)) {
      const keys = Object.keys(obj);
      for (let i = 0, ilen = keys.length; i < ilen; ++i) {
        const key = keys[i];
        const it = obj[key];
        sb.append(`\n${indent}${key}`);
        if (i === 0)
          first = key;
        if (nx.isObject(it) || nx.isArray(it) || nx.isFunction(it))
          sb.append(` ${FindMisses(it, indent + '  ', depth)}`);
      }
    } else if (nx.isArray(obj)) {
      for (let i = 0, ilen = obj.length; i < ilen; ++i) {
        const it = obj[i];
        sb.append(`\n${indent}${it}`);
        if (i === 0)
          first = it;
        if (nx.isObject(it) || nx.isArray(it) || nx.isFunction(it))
          sb.append(` ${FindMisses(it, indent + '  ', depth)}`);
      }
    } else if (nx.isFunction(obj)) {
      const opts = obj({help: true});		// ask function for options
      if (opts.obj)
        sb.append(` ${FindMisses(opts.obj, indent + '  ', depth)}`);
    }
  }

  if (sb._array.length !== 1)
    return sb.toString();
  if (first)
    return `${first.toString()} `;
  return ' ';
}


function DoCommand(ocmd, cmd, cmds, response) {

  Log(`Command: ${ocmd}`);

  let hits = FindHits(cmd, cmds, 'isFunction');		// find functions

  if (hits.length === 0) {
    if (cmd.length && 'help'.indexOf(cmd) < 0)
      return response.append(`I cannot do ${ocmd}\nTry: ${FindMisses(cmds, '  ')}`);
    return response.append(`Help: ${FindMisses(cmds, '  ')}`);
  }

  if (hits.length > 1)     // more than one hit, need to narrow it down
    return response.append(`Try: ${FindMisses(hits, '  ')}`);

  if (!hits.matched)   // The hit did not resolve to a command function
    return response.append(`Try: ${FindMisses(hits, '  ')}`); // give some options

  response.insert(0, `${hits.cmdString}:\n`);		// insert the command name


  const cmdFunction = hits.matched.val;			// the command function

// we have the command, lets try to parse its options
  if (hits.argv.tail.length === 0)  // no command tail
    return cmdFunction({hits: hits, response: response})    // call the command

  // parse the tail
  let opts = cmdFunction({help: true, hits: hits, response: response});
  if (opts.obj) {
    opts = FindHits(hits.argv.tail, opts.obj, opts.typ, hits);
    response.replace(0, `${opts.cmdString}:\n`);		// insert the command name

    if (opts.matched)
      return cmdFunction({hits: opts, response: response})    // call the command
    else if (opts.length)
      return response.appendLine(`Try: ${opts.cmdString} ${FindMisses(opts, '  ')}`);
  }

  return cmdFunction({hits: hits, response: response})    // call the command without options
}


function ChatCommand(context, cmd, cb) {

  const cmds =
    {
      sim:
        function (opts) {
          const myOpts = {};

          if (opts.help)
            return myOpts;

          const response = opts.response;
          const tail = opts.hits.argv.tail.trim();
          if (!tail || tail.length === 0)
            return response.appendLine('no question given');
          return SelectSimilarity(tail, response);
        },
      sum:
        function (opts) {
          const myOpts = {};

          if (opts.help)
            return myOpts;

          const response = opts.response;
          SummarizeConversation(context, function (err, result) {
            if (err)
              return LogError(err.toString());
            Log(result.response.text);
          });
          return response.appendLine('running');
        },
      show:
        function (opts) {
          const myOpts = {obj: context, typ: ['isObject', 'isArray']};

          if (opts.help)
            return myOpts;

          const response = opts.response;
          const hits = opts.hits;
          if (!hits || !hits.matched)
            return response.appendLine(`Parsing error: ${stringify(opts)}`);

          const key = hits.matched.key;
          const it = hits.matched.hit[key];
          if (!it)
            return response.appendLine(`there are no ${key} to show`);

// show result
          response.appendLine(`${key}: ${stringify(it, null, 4)}`);
          response.appendLine(`${it.length} items`);
        },
      list:
        {
          session: function (opts) {
            const myOpts = {};

            if (opts.help)
              return myOpts;

            const response = opts.response;
            const session = nx.CloneObj(context.options.etc.session);
            if (nx.resolveProperty(session, 'model.history'))
              delete session.model.history;
            response.appendLine(stringify(session, null, 4));
          }
        },
      purge:
        function (opts) {
          const myOpts = {obj: context.history, typ: 'isArray'};

          if (opts.help)
            return myOpts;

          const response = opts.response;
          const tail = opts.hits.argv.tail.trim();
          context.history = [];
          context.model.history = [];

          return response.appendLine('purged');
        },
      set:
        function (opts) {
          const myOpts = {obj: context.model.dialog, typ: ['isString', 'isBoolean', 'isNumber']};

          if (opts.help)
            return myOpts;

          const response = opts.response;
          const tail = opts.hits.argv.tail.trim();
          if (tail.length === 0)
            return response.appendLine('no value given');

          const key = opts.hits.matched.key;
          opts.hits.matched.hit[key] = tail;

          return response.appendLine(`${key} set to ${tail}`);
        },
      enable:
        function (opts) {
          const myOpts = {obj: context.options.enabled, typ: 'isBoolean'};

          if (opts.help)
            return myOpts;

          const response = opts.response;
          const tail = opts.hits.argv.tail.trim();
          const hits = FindHits(tail, myOpts.obj, myOpts.typ);
          if (!hits.matched) {
            if (hits.length)
              return response.appendLine(`Try: ${opts.cmdString} ${FindMisses(hits, '  ')}`);
            return response.appendLine(`Try: ${opts.cmdString} ${FindMisses(myOpts.obj, '  ', 1)}`);
          }

          const key = hits.matched.key;
          hits.matched.hit[key] = true;

          return response.appendLine(`${key} enabled`);
        },
      disable:
        function (opts) {
          const myOpts = {obj: context.options.enabled, typ: 'isBoolean'};

          if (opts.help)
            return myOpts;

          const response = opts.response;
          const tail = opts.hits.argv.tail.trim();
          const hits = FindHits(tail, myOpts.obj, myOpts.typ);
          if (!hits.matched) {
            if (hits.length)
              return response.appendLine(`Try: ${opts.cmdString} ${FindMisses(hits, '  ')}`);
            return response.appendLine(`Try: ${opts.cmdString} ${FindMisses(myOpts.obj, '  ', 1)}`);
          }

          const key = hits.matched.key;
          hits.matched.hit[key] = false;

          return response.appendLine(`${key} disabled`);
        },
      debug:
        function (opts) {
          const myOpts = {obj: context.options.debug, typ: 'isBoolean'};

          if (opts.help)
            return myOpts;

          const response = opts.response;
          const tail = opts.hits.argv.tail.trim();
          const hits = FindHits(tail, myOpts.obj, myOpts.typ);
          if (!hits.matched) {
            if (hits.length)
              return response.appendLine(`Try: ${opts.cmdString} ${FindMisses(hits, '  ')}`);
            return response.appendLine(`Try: ${opts.cmdString} ${FindMisses(myOpts.obj, '  ', 1)}`);
          }

          const key = hits.matched.key;
          hits.matched.hit[key] = true;

          return response.appendLine(`debug for ${key} enabled`);
        },
      nodebug:
        function (opts) {
          const myOpts = {obj: context.options.debug, typ: 'isBoolean'};

          if (opts.help)
            return myOpts;

          const response = opts.response;
          const tail = opts.hits.argv.tail.trim();
          const hits = FindHits(tail, myOpts.obj, myOpts.typ);
          if (!hits.matched) {
            if (hits.length)
              return response.appendLine(`Try: ${opts.cmdString} ${FindMisses(hits, '  ')}`);
            return response.appendLine(`Try: ${opts.cmdString} ${FindMisses(myOpts.obj, '  ', 1)}`);
          }

          const key = hits.matched.key;
          hits.matched.hit[key] = false;

          return response.appendLine(`debug for ${key} disabled`);
        }
    };

  const response = new nx.StringBuilder();
  DoCommand(cmd, cmd, cmds, response);
  cb(null, '\n' + response.toString());
}


function ChatExit(context, status) {
  let mods = 0;

  LogWrite('\nExiting... ');

  if (context) {
    if (nx.resolveProperty(context, 'enabled.save.context')) {
      Log(`Saving full context`);
      Merge(context, ChatConfig);
      delete context.options;
      nx.putEnv('chatgpt.chat', context);
      ++mods;
    } else if (nx.resolveProperty(context, 'enabled.save.history')) {
      Log(`Saving ${context.history.length} history items`);
      nx.putEnv('chatgpt.chat.history', context.history);
      ++mods;
    }
  }

  if (!mods)
    Log('nothing saved');

  LogWrite('\n');
  process.exit(status);
}


function QuizLoop(context, input) {

  context = BuildContext(context);

  if (!input)
    input = process.stdin;

  if (!input.isTTY)
    return Ask(context, fs.readFileSync(input.fd).toString().split('\n'));

  const rl = readline.createInterface(input, process.stdout);
  const wrap = nx.resolveProperty(context, `options.log.wrap`);

  if (wrap)
    wrap.cols = process.stdout.columns;

  Log('Ready');
  rl.on('line', (question) => {
    question = question.trim();

    if (question.length <= 0 || question === 'quit')
      ChatExit(context, 0);

    Ask(context, question, undefined, {echo: false, wrap: wrap});
  });
}


function main(argv) {

// house keeping
  delete Config.chat;
  Config.files = {};    // collection of open files

  const context = BuildContext();

  const history = nx.parseJsonFile('history.json');
  for (let i = 0; i < history.length; ++i) SaveHistory(context, history[i].user, history[i].bot);

  while (argv.length) {
    if (argv[0] === '-w')
      return StartWebService(argv.slice(1));

    if (argv[0] === '-i' && argv.length > 1) {
      argv.shift();
      const file = argv.shift();
      Ask(context, fs.readFileSync(file).toString().split('\n'));
      continue;
    }

    const arg = argv.join(' ').trim();
    if (arg.length)
      return Ask(context, arg);
  }

  // const e = ElementString('String_Cosine', String_Cosine);
  // return Log(e);

  // const history = SelectHistory(context, 'what were the actors in the movie');
  // Log(history);
  // return Log(history.length);

  //return console.log(stringify(history));

  // return Ask(context, ['.', '.h', '.se', '.s', '.ses', '.seh', '.sde', '.debug proto']);

  // Ask(context, 'is the term lent out of date?');

  // Ask(context,'.sum');

  // Ask(context, 'have we spoken of flashlights');

  // Ask(context, 'for how long has afghanistan been afghanistan?');

  return QuizLoop(context);
}

main(process.argv.slice(2));
