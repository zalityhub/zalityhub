const fs = require('fs');
const readline = require('readline');
const stringify = require('json-stringify-safe');
const linewrap = require('linewrap');
const uuid = require('uuid');

const nx = require('../util/util.js');
const Config = nx.getEnv(['chatgpt'], true);
const ChatConfig = nx.CloneObj(Config.chat_service);
const Merge = require('lodash.merge');


function CheckPoint(objs) {
return;
    objs.forEach((entry) => {
      if(!global[entry.name])
        global[entry.name] = [];
      global[entry.name].ref = entry.obj;
      global[entry.name].push(nx.CloneObj(Merge({stack: (new Error().stack).split('\n')}, entry)));
    });
}


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


function ChatLog(text) {
  nx.writeHandle(Config.context.settings.log, text);
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
    (!(handle = nx.resolveProperty(context, `settings.debug.${handle}`.split('.')))))
    return false;
  if (!handle.enabled)
    return false;
  if (!text)
    return true;    // is enabled

  if (nx.isArray(text)) {
    const sb = new nx.StringBuilder();
    for (let i = 0; i < text.length; ++i)
      sb.push_line(text[i]);
    return Debug(context, handle, sb.toString());
  }

  if (!Object.entries(handle.pipes).length && context.settings.log.copydebug)    // not pipeing & copy debug enabled
    Log(text);    // repeat to console
  if (!Object.entries(handle.pipes).length && context.settings.log.copydebug)    // not pipeing & copy debug enabled
    Log(text);    // repeat to console

  nx.writeHandle(handle, `${text}\n`);

  nx.pipe(context.settings.debug.pipes, text);
  return text;
}


function ElementString(name, el, indent, depth) {

// Todo: need to detect looping
  return '';

  if(nx.isNull(depth))
    depth = 0;

  if(++depth > 10)
    nx.fatal('looping');

  indent = indent ? indent : '';
  const sb = new nx.StringBuilder();

  if (nx.isNull(el)) {
    return `${indent + name}(null): ${el}`;
  }
  if (nx.isObject(el)) {
    sb.push_line(indent + name + ' {');
    const keys = Object.keys(el);
    for (let i = 0, ilen = keys.length; i < ilen; ++i) {
      const key = keys[i];
      const text = ElementString(key, el[key], indent + '  ', depth);
      sb.push(text);
    }
    sb.push_line(indent + '}');
    return sb.toString();
  }
  if (nx.isArray(el)) {
    sb.push_line(indent + name + ' [');
    for (let i = 0, ilen = el.length; i < ilen; ++i) {
      const key = i.toString();
      const text = ElementString(key, el[i], indent + '  ', depth);
      sb.push(text);
    }
    sb.push_line(indent + ']');
    return sb.toString();
  }
  if (nx.isStringBuilder(el)) {
    const text = ElementString(name, el.array, indent, depth);
    sb.push(text);
    return sb.toString();
  }
  if (nx.isFunction(el)) {
    sb.push(el.toString());
    return sb.toString();
  }
  const typ = typeof el;
  sb.push_line(`${indent + name}(${typ}): "${el.toString()}"`);
  return sb.toString();
}


function BuildPaths(context) {
  const paths = [];

  function build(base, path, el) {
    paths.push(path);
  }

  nx.Iterate('', context, build);
  return paths;
}


function NewSession(session) {
  if(!session)
    session = {uuid: uuid.v4()};
  session = Merge(session, {date: `${nx.date()}`, log: [], errors: []});    // reset
  return session;
}

function GetSettings(context) {

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
      if (!properties[i])    // if no prior array
        context[key] = [].concat(ChatConfig[key]);    // copy from the base
    } else if (nx.isObject(ChatConfig[key])) {
      context[key] = nx.CloneObj(ChatConfig[key]);
      if (properties[i])    // if passed an object
        Merge(context[key], properties[i]);
    } else if (nx.isString(ChatConfig[key])) {
      if (!properties[i])    // if not passed a string
        context[key] = (' ' + ChatConfig[key]).slice(1);    // force a copy
    }
  }

// prepare settings
  if(!context.inited) {
    const settings = context.settings;

    context.uuid = uuid.v4();

    context.session = NewSession();

    // set up debug/log pipes
    Object.keys(settings.debug).forEach(function (key) {
      settings.debug[key].pipes = {};
    });
    settings.log.pipes = {};
    settings.debug.pipes = {};

    let choice = Choose('model', settings.model_methods); // choose the model
    context.model = choice.item;
    choice = Choose('match_method', settings.match_methods); // choose the matching method
    context.match_method = choice.item;
    context.match_method.method = choice.name;
    context.inited = true;
  }

// convert strings to numerics...
  ['temperature',
    'max_tokens',
    'top_p',
    'frequency_penalty',
    'presence_penalty'].forEach((key) => {
    if (nx.isString(context.model.dialog[key]))
      context.model.dialog[key] = parseFloat(context.model.dialog[key]);
  });

  Debug(context, 'context', ElementString('context', context));

  // context.paths = BuildPaths(context).sort();

  context._do_not_clone_me = true;
  return context;
}


function Choose(name, choices) {
  let choice = choices.choose;
  if (nx.isArray(choice))
    choice = choice[0];    // use the first one

  let item = choices.items[choice];
  if (!item) {
    Log(`${name} ${choice} is not available`);
    choice = Object.entries(choices.items)[0];
    item = choice[1];   // here's the first occurring object
    choice = choice[0]; // and its name
  }
  Log(`Using ${name} ${choice}`);
  return {name: choice, item: item};
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


function SendChatReq(options, callback) {

  function logSession() {
    if (Debug(context, 'session')) 
      return Debug(context, 'session', `session=${stringify(session, null, 4)}`);
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

  function callbacker(err, context) {
    if (!callback)
      callback = function () {
      };

    if (err) {
      sLogError(err.toString());
      context.session.error = err;
    }

    logSession();
    callback(err, context);
  }


  function requester(cb) {
    const protocol = require(model.config.type);  // http or https
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

    function buildPrompt() {
      switch (session.dialog.model) {
        default:
          session.prompt = [];
          while (matched.length) {
            const h = nx.CloneObj(matched.shift());
            if (h.user && h.user.length) {
              session.prompt.push({role: user, content: h.user});
              session.word_count += h.user.split(' ').length;
            }
            if (h.bot && h.bot.length) {
              session.prompt.push({role: system, content: h.bot});
              session.word_count += h.bot.split(' ').length;
            }
          }
          session.dialog.messages = session.prompt;    // pass the prompt
          break;

        case 'text-davinci-003':
          session.prompt = new nx.StringBuilder();
          while (matched.length) {
            const h = nx.CloneObj(matched.shift());
            if (h.user && h.user.length) {
              session.prompt.push(`${user}: ${h.user}\n`);
              session.word_count += h.user.split(' ').length;
            }
            if (h.bot && h.bot.length) {
              session.prompt.push(`${system}: ${h.bot}\n`);
              session.word_count += h.bot.split(' ').length;
            }
          }
          session.dialog.prompt = session.prompt.toString();    // pass the prompt
          break;
      }

      return session.prompt;
    }


// Make a copy of the dialog and matched history object
    session.dialog = nx.CloneObj(model.dialog);
    const system = session.dialog.system;
    const user = session.dialog.user
    delete session.dialog.system;
    const matched = nx.CloneObj(session.matched.items);

    session.word_count = 0;
    session.token_count = 0;

    buildPrompt();

    session.token_count = Math.floor(session.word_count * model.options.token_ratio);
    sDebug('logic', `Using a token_ratio of ${model.options.token_ratio}, ${session.word_count} words makes ${session.token_count} tokens`);
    if ((session.dialog.max_tokens -= session.token_count) < 100) {
      session.dialog.error = `model ${session.dialog.model} max_token ${session.dialog.max_tokens} too small`;
      sLogError(session.dialog.error);
      const error = {
        error: true,
        error_type: 'max_token_too_small',
        text: session.dialog.error,
        dialog: session.dialog
      };
      return error;
    }

    return {error: false, dialog: session.dialog};
  }

  function buildReq() {
    model.config.headers.Authorization = ChatConfig.settings.model_methods.Authorization;
    if (enabled.history)
      session.matched = Query.select(context.session.options.question, context.match_method);
    else
      session.matched = {using: nx.CloneObj(context.match_method), items: []};
    session.matched = nx.CloneObj(session.matched);

    Query.save({bot: model.options.personality}, session.matched.items);    // add personality
    Query.save({user: context.session.options.question}, session.matched.items);    // add this question

    const result = buildDialog();

// todo: put retry logic here: 'max_token_too_small'

    if (result.error)
      return result;

    result.reqData = stringify(result.dialog);
    return result;
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

      model.options.token_ratio *= 1.1;
      context.settings.similar.min_ratio *= 1.1;
      sLog(`Retrying to answer '${context.session.options.question}' with token ratio of ${model.options.token_ratio}`);
      logSession();
      session = context.session = NewSession();
      return true;      // say retry
    }

    return false;     // no retry
  }

  function updateRatios() {
    const usage = nx.resolveProperty(context, 'response.json.usage'.split('.'));
    if (!usage)
      return;   // none

    sDebug('logic', `Reported usage: ${stringify(usage, null, 4)}`);

    const token_ratio = usage.prompt_tokens / session.word_count;
    if (token_ratio < model.options.token_ratio)
      return;   // never regress

    let delta = token_ratio / model.options.token_ratio;
    if (delta > 1.0)
      delta = 1.0 / delta;
    delta = Math.floor((1.0 - delta) * 100.0);
    const msg = `Used token_ratio: ${model.options.token_ratio}\n` +
      `Reported token_ratio: ${token_ratio}\n` +
      `Delta diff: ${delta}%`;
    sDebug('logic', msg);
    if (delta > 10) {    // update the model
      sLog(msg + '\nUpdating the model');
      model.options.token_ratio = token_ratio;
    }
  }

  function receiver(err, body) {
    if (nx.resolveProperty(model, 'config.headers.Authorization'.split('.')))
      delete model.config.headers.Authorization;    // remove the key

    if (err)
      return handleError(err);

    session.body = nx.replaceAll(body.toString().trim(), ['assistant:', 'bot:'], ['', ''], true);
    context.response = ParseGptResponse(JSON.parse(session.body));
    session.response = context.response;

    if (nx.resolveProperty(context, 'response.json.error.message'.split('.')))
      return handleError(context.response.json.error.message);

    ChatLog(`Response: [${stringify(context.response.text)}]\n`);
    Query.save({user: context.session.options.question, bot: context.response.text});

    updateRatios();

    return callbacker(null, context);
  }


// SendChatReq function begins here
//
// short cuts...
  const context = GetSettings(options.context);
  const settings = context.settings;
  const enabled = settings.enabled;
  const model = context.model;

  let session = context.session = NewSession();
  session.options = options;
  session.model = model;

  try {
    session.options.question = (!session.options.question) ? '' : session.options.question.toString().trim();
    if (!session.options.question.length) {
      const err = new Error('no question given');
      return callbacker(err);
    }

    ChatLog(`Question: [${context.session.options.question}]\n`);   // put in the log

    sDebug('logic', `Using model ${model.dialog.model}`);

    // build request data
    const request = buildReq();

    if (request.error) {
      if (request.error_type === 'max_token_too_small')
        sLogError(`${stringify(session.matched, null, 4)}\nIs too much history`);
      return callbacker(request.error_type);
    }

    // get the http request
    const req = requester(receiver);

// then send it
    req.write(request.reqData);
    req.end();
  } catch (err) {
    callbacker(err);
  }
}


function ChatCommand(options, cmd, cb) {

  const cmds =
    {
      list:
        function (opts) {
          const my_opts = {obj: context, typ: ['isAny']};

          if (opts.help)
            return my_opts;

          const hits = opts.hits;
          const response = opts.response;
          opts = Command.FindHits({chase: true, args: hits.argv.tail, obj: my_opts.obj, typ: my_opts.typ});
          if (!opts.matched)
            return response.push_line(`Try: ${hits.cmdString} ${Command.FindMisses(opts, '  ')}`);
          response.write(`${opts.matched.key}:\n${stringify(opts.matched.obj, null, 4)}`);
        },
      modify:
        function (opts) {
          const my_opts = {obj: context, typ: ['isAny']};

          if (opts.help)
            return my_opts;
        },
      purge:
        function (opts) {
          const my_opts = {obj: Query._history, typ: 'isArray'};

          if (opts.help)
            return my_opts;
        }
    };

  const response = new nx.StringBuilder();
  const context = options.context;
  Command.DoCommand(cmd, cmd, cmds, response);
  cb(null, '\n' + response.toString());
}


function Ask(options, callback) {

  function ask(q, cb) {
    if (q.length) {
      if (options.echo)
        sb.push_line(`\n${q}\n`);
      if (options.context.settings.enabled.commands && q.indexOf('.') === 0) {
        return ChatCommand(options, q.slice(1).trim(), cb);
      }
      options.question = q;
      return SendChatReq(options, cb);
    }
    nextAsk();    // try next
  }

  function nextAsk() {
    if (!options.questions.length)
      return callback(null, sb.toString());

    const q = options.questions.shift();
    if(!q) {
      sb.push_line(`\nNo question!\n`);
        return nextAsk();    // send next
      }

    ask(q.trim(),
      function (err, result) {
        if (err) {
          err = `\n${err.toString()}\n`;
          sb.push_line(err);
          return callback(err, sb.toString());
        }

        if (nx.resolveProperty(result, 'response.text'.split('.'))) {
          let text = result.response.text;
          if(options.wrap) 
            text = WrapWords(text, options.wrap);
          sb.push_line(text);
        } else {
          sb.push_line(`\n${result.toString()}\n`);
          }

        nextAsk();    // send next
      });
  }

  if (!nx.isArray(options.questions))
    options.questions = [options.questions];
  else
    options.questions = [].concat(options.questions);

  if(options.stream)
    options.questions = [options.questions.join('\n')];

  const sb = new nx.StringBuilder();

  callback = callback ? callback : function (err, result) {
    if (err)
      return Log(err, options);

    LogWrite('\n');
    Log(result);
    LogWrite('\n');
  return ChatLog(stringify(options, null, 4));
  };

  nextAsk();
}


function StartWebService(argv) {

  const express = require('express');
  const cors = require('cors');
  const Url = require('url');

  argv = argv.join('');
  if (argv.length <= 0)
    argv = Config.proxy_service.proxyServerUrl;   // no arg, use the configured url

  const url = Url.parse(argv);

  const app = express();
  app.use(express.json());
  app.use(cors())
  app.get(`${url.path}*`, (req, res) => {
    try {
      let question = req.url.slice(url.path.length);
      if (question.charAt(0) === '?' || question.charAt(0) === '&')
        question = question.slice(Math.max(1, question.indexOf('=') + 1));

      Ask({questions: question}, function (err, result) {
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
      const context = GetSettings(JSON.parse(body));
      Ask({context: context, questions: context.question}, function (err, result) {
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


function ChatExit(context, status) {
  let mods = 0;

  LogWrite('\nExiting... ');

  if (context) {
    if (nx.resolveProperty(context, 'enabled.save.context'.split('.'))) {
      Log(`Saving full context`);
      Merge(context, ChatConfig);
      nx.putEnv('chatgpt.chat_service', context);
      ++mods;
    } else if (nx.resolveProperty(context, 'enabled.save.history'.split('.'))) {
      Log(`Saving ${Query._history.length} history items`);
      nx.putEnv('chatgpt.chat_service.history', Query._history);
      ++mods;
    }
  }

  if (!mods)
    Log('nothing saved');

  LogWrite('\n');

  ChatLog(`global=${stringify(global)}`);
  process.exit(status);
}


function QuizLoop(options) {

  CheckPoint([ {name: 'options', obj: options} ]);

  if(!options.context)
    options.context = GetSettings();

  if (!options.input)
    options.input = process.stdin;

  if (!options.input.isTTY) {
    options.questions = fs.readFileSync(options.input.fd).toString().split('\n');
    return Ask(options);
  }

  if(options.context.settings.wrap) {
    const wrap = options.context.settings.wrap;
    wrap.cols = process.stdout.columns;
    options = {...{wrap: wrap}, ...options};
  }

  const rl = readline.createInterface(options.input, process.stdout);

  Log('Ready');
  rl.on('line', (question) => {
    question = question.trim();

    if (question.length <= 0 || question === 'quit' || question === 'q' )
      ChatExit(options.context, 0);

    options.questions = question;
    Ask(options);
  });
}


function main(argv) {

// house keeping

  global = {seq: 0};

  delete Config.chat_service;
  Config.files = {};    // collection of open files

  const context = GetSettings();

  query = require('./query.js');
  Query = new query.Query('history.json');
  // Query = new query.Query();

  command = require('./command.js');
  Command = new command.Command();

  const ask_options = {context: context};

  while (argv.length) {
    if (argv[0] === '-s')
      ask_options.stream = true;

    else if (argv[0] === '-w')
      return StartWebService(argv.slice(1));

    else if (argv[0] === '-i' && argv.length > 1) {
      argv.shift();
      const file = argv.shift();
      Ask({...ask_options, ...{echo: true, questions: fs.readFileSync(file).toString().split('\n')}});
      continue;
    }
    else {
      const arg = argv.join(' ').trim();
      if (arg.length)
        return Ask({...ask_options, ...{questions: arg}});
    }
    argv.shift();
  }

  // const matched = SelectHistory(context, 'what were the actors in the movie');
  // Log(matched);
  // return Log(matched.length);

  //return console.log(stringify(matched));

  // return Ask({...ask_options, ...{questions: 'show me a list of six characters words ending in beach'}});

// return Ask({...ask_options, ...{questions: ['.', '.h', '.se', '.s', '.ses', '.seh', '.sde', '.debug proto']});

  // Ask({...ask_options, ...{questions: 'is the term lent out of date?'}});

  // Ask({...ask_options, ...{questions: 'have we spoken of flashlights'}});

  // Ask({...ask_options, ...{questions: 'for how long has afghanistan been afghanistan?'}});

  // return Ask({...ask_options, ...{questions: '.l s e'}});
  // return Ask({...ask_options, ...{questions: '.l s'}});

  return QuizLoop(ask_options);
}

main(process.argv.slice(2));
