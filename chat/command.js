exports.Command = class Command {

  stringify = require('json-stringify-safe');

  nx = require('../util/util.js');


  constructor(history) {
    const self = this;
  }

FindMisses(obj, indent, depth) {
    const self = this;
  indent = indent ? indent : '';

  if (depth === undefined || depth === null)
    depth = 16;			// arbitrary depth

  if (--depth < 0)
    return '';				// done

  const sb = new self.nx.StringBuilder();
  let first;

  if (obj.hits) {   // these are from hits...
    const hits = obj.hits;
    for (let i = 0, ilen = hits.length; i < ilen; ++i) {
      const hit = hits[i];
      if (i === 0)
        first = hit.key;
      sb.push(`\n${indent}${hit.key}`);
      if (hit.what.isObject)
        sb.push(` ${self.FindMisses(hit.val, indent + '  ', depth)}`);
    }
  } else {
    if (self.nx.isObject(obj)) {
      const keys = Object.keys(obj);
      for (let i = 0, ilen = keys.length; i < ilen; ++i) {
        const key = keys[i];
        const it = obj[key];
        sb.push(`\n${indent}${key}`);
        if (i === 0)
          first = key;
        if (self.nx.isObject(it) || self.nx.isArray(it) || self.nx.isFunction(it))
          sb.push(` ${self.FindMisses(it, indent + '  ', depth)}`);
      }
    } else if (self.nx.isArray(obj)) {
      for (let i = 0, ilen = obj.length; i < ilen; ++i) {
        const it = obj[i];
        sb.push(`\n${indent}${it}`);
        if (i === 0)
          first = it;
        if (self.nx.isObject(it) || self.nx.isArray(it) || self.nx.isFunction(it))
          sb.push(` ${self.FindMisses(it, indent + '  ', depth)}`);
      }
    } else if (self.nx.isFunction(obj)) {
      const opts = obj({help: true});		// ask function for options
      if (opts.obj)
        sb.push(` ${self.FindMisses(opts.obj, indent + '  ', depth)}`);
    }
  }

  if (sb._array.length !== 1)
    return sb.toString();
  if (first)
    return `${first.toString()} `;
  return ' ';
}

  FindHits(opts) {
    const self = this;

    function getCmdString(result) {
      const sb = new self.nx.StringBuilder();
      while (result && result.length === 1) {
        sb.unshift(result.hits[0].key);
        result = result.prior;
      }
      return sb.toString({post: ' '}).trim();
    }


    function buildResult() {
      let arg = opts.args = (' ' + opts.args).slice(1).trim();    // force a copy
      const i = arg.indexOf(' ');   // first space
      if (i > 0)
        arg = arg.slice(0, i).trim();

      const argv = {args: opts.args, arg: arg, tail: opts.args.slice(arg.length + 1).trim()};
      const result = {matched: false, hits: [], length: 0, argv: argv, cmdString: ''};

      argv.oargs = opts.args;					// save the arg string
      return result;
    }


    const result = buildResult();
    const argv = result.argv;
    const arg = argv.arg;
    const obj = opts.obj;
    const typ = opts.typ;
    const hits = result.hits;

    if (arg.length) {

// find matching keys
      const keys = Object.keys(obj);
      for (let i = 0, ilen = keys.length; i < ilen; ++i) {
        const key = keys[i];
        if (key.indexOf(arg) === 0)
          hits.push({hit: obj, obj: obj[key], key: key, what: self.nx.isWhat(obj[key])});
      }

      result.hits = hits;
      result.length = hits.length;
      result.cmdString = getCmdString(result);

      if (hits.length === 1) {			// a unique match
        if (hits[0].what.is(typ)) {      // and the desired type
          if (!opts.chase || argv.tail.length === 0) {
            result.matched = hits[0];		// Bingo
            return result;
          }
          const r = self.FindHits({chase: true, args: argv.tail, obj: hits[0].obj, typ: typ});
          return r;
        }
      }
    }

    result.missed = opts;
    return result;
  }

  DoCommand(ocmd, cmd, cmds, response) {
    const self = this;

    let hits = self.FindHits({args: cmd, obj: cmds, typ: 'isFunction'});		// find functions

    if (hits.length === 0) {
      if (cmd.length && 'help'.indexOf(cmd) < 0)
        return response.push(`I cannot do ${ocmd}\nTry: ${self.FindMisses(cmds, '  ')}`);
      return response.push(`Help: ${self.FindMisses(cmds, '  ')}`);
    }

    if (hits.length > 1)     // more than one hit, need to narrow it down
      return response.push(`Try: ${self.FindMisses(hits, '  ')}`);

    if (!hits.matched)   // The hit did not resolve to a command function
      return response.push(`Try: ${self.FindMisses(hits, '  ')}`); // give some options

// we have the command

    const cmdFunction = hits.matched.obj;			// the command function
    return cmdFunction({hits: hits, response: response});   // call the command 
  }
}
