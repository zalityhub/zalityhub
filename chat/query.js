exports.Query = class Query {

  fs = require('fs');
  uuid = require('uuid');
  Corpus = require('./Corpus.js');
  Pluralize = require('pluralize')
  readline = require('readline');

  nx = require('../util/util.js');

  Stats = require('./stats.js');

  history = [];
  txtSeen = [];
  uidSeen = new Set();

  constructor(history) {
    const self = this;

    if (history)
      self.load(history);

    self.stats = new self.Stats.Stats(function (it, self) {
      if (!it)
        return 0.0;
      return it.ratio;
    });
  }

  clear(history) {
    const self = this;

    if (!history)
      history = self.history;
    history.length = 0;
  }

  load(history) {
    const self = this;

    if (!history)
      history = [];

    if (self.nx.isString(history))
      history = self.nx.parseJsonFile(history);

    history.forEach((entry) => {
      self.save(entry);
    });
  }

  save(entry, history) {
    const self = this;

    if (!history)
      history = self.history;

    if (!entry.uuid)
      entry.uuid = self.uuid.v4();
    if (!entry.date)
      entry.date = self.nx.date();
    if (!entry.bot)
      entry.bot = '';
    if (!entry.user)
      entry.user = '';
    if (!entry.ratio)
      entry.ratio = 0.0;

    entry.bot = self.nx.replaceAll(entry.bot.trim(), ['assistant:', 'bot:'], ['', ''], true);
    entry.user = self.nx.replaceAll(entry.user.trim(), ['assistant:', 'bot:'], ['', ''], true);
    history.push(entry);
  }


  getSimilar(text, options, history) {
    const self = this;

    if (!history)
      history = self.history;

    if (!options)
      options = {method: 'tf-idf'};

    const map = {};
    const h1 = [];
    const h2 = [];

    history.forEach((entry) => {
      map[entry.uuid] = entry;

      if (entry.user) {
        h1.push(`user.${entry.uuid}`);
        h2.push(entry.user.trim().toLowerCase());
      }
      if (entry.bot) {
        h1.push(`bot.${entry.uuid}`);
        h2.push(entry.bot.trim().toLowerCase());
      }
    });

    const corpus = new self.Corpus.Corpus(h1, h2);

    function pluralize(text) {
      text = text.split(' ');   // words
      for (let i = 0; i < text.length; ++i)
        text[i] = self.Pluralize(text[i]);
      return text.join(' ');
    }

    function singularize(text) {
      text = text.split(' ');   // words
      for (let i = 0; i < text.length; ++i)
        text[i] = self.Pluralize.singular(text[i]);
      return text.join(' ');
    }

    function getHits(text) {
      const results = [];
      const hits = corpus.getResultsForQuery(text);

      hits.forEach((hit) => {
        const ratio = hit[1];
        if (!options.min_ratio || ratio >= options.min_ratio) {
          let uid = hit[0];
          const d = uid.indexOf('.');
          const role = uid.slice(0, d);
          uid = uid.slice(d + 1);
          const entry = self.nx.CloneObj(map[uid]);
          entry.method = options.method;
          entry.role = role;
          entry.ratio = ratio;
          entry.hit = text;
          entry.on = entry[role];
          results.push(entry);
        }
      });
      return results;
    }

    let results = [];
    results = results.concat(getHits(text));
    results = results.concat(getHits(singularize(text)));
    results = results.concat(getHits(pluralize(text)));
    return self.prune(results);
  }

  select(text, options, history) {
    const self = this;

    function save_stats(text, matched, size) {
      const stats = {};
return stats;

// todo: needs work
//
      stats.matched = `${text} matched ${matched.length} from ${size}`;
      stats.items = [];
      matched.forEach((entry) => {
        stats.items.push(`ratio: ${entry.ratio}, uuid: ${entry.uuid}\non ${entry.role}: ${entry.on}`);
      });
      stats.percentiles = [];
      status.percentiles.push(`median:  ${stats.median(matched)}`);
      [10, 25, 50, 75, 80, 90].forEach((percentile) => {
        status.percentiles.push(`${percentile}%:     ${stats.calc(matched, percentile / 100.0)}`);
      });

      return stats;
    }

    if (!options)
      options = {};

    if (!history)
      history = self.history;

    text = text.toString().trim();

    text = text.trim().toLowerCase();

    let matched = self.getSimilar(text, options, history);

    if (options.min_history) {
      const last = self.nx.CloneObj(history.slice(0 - options.min_history)).map(v => {
        return {...v, ...{method: 'last', ratio: 0}};    // get last N
      });
      matched = self.prune(matched.concat(last));
    }

    matched = self.nx.CloneObj(matched);

    if(options.save_stats)
      matched.stats = save_stats(text, matched, history.length);

    return {using: self.nx.CloneObj(options), items: matched};
  }

  seenText(text) {
    const self = this;

    if (!text)
      return false;
    text = text.toString().toLowerCase().trim();
    if (text.length === 0)
      return false;
    self.txtSeen.forEach((txt) => {
      if (text === txt)
        return true;
    });
    self.txtSeen.push(text);
    return false;
  }

  sanitize(entry) {
    const self = this;

    if (self.uidSeen.has(entry.uuid))
      return false;   // this is a dup, don't add

    if (self.seenText(entry.bot))
      entry.bot = '';
    if (self.seenText(entry.user))
      entry.user = '';

    if (entry.user.length === 0 && entry.bot.length === 0)
      return false;   // missing bot and user text

    self.uidSeen.add(entry.uuid);
    return true;   // add
  }

  sort(history) {
    const self = this;
    if (!history)
      history = self.history;

    return history.sort(function (a, b) {     // sort by ratio
      return a.ratio - b.ratio;
    });
  }

  prune(history) {
    const self = this;

    if (!history)
      history = self.history;

    const nodups = [];
    self.txtSeen = [];
    self.uidSeen = new Set();

// skips any entries already present (uuid)
// removes any user or bot text already present

    history.forEach((entry) => {
      if (self.sanitize(entry))
        nodups.push(self.nx.CloneObj(entry));
    });

    return self.sort(nodups);
  }

  reduce(percentile, history, cb) {
    const self = this;

    if (!history)
      history = self.history;

    const ratio = self.stats.calc(history, percentile / 100.0);

    const reduced = [];

    history.forEach((entry) => {
      if (entry.ratio >= ratio)
        reduced.push(self.nx.CloneObj(entry));
      else if (cb)
        cb(entry, ratio);   // show discards
    });

    return self.sort(reduced);
  }


  static Test(argv, history) {
    if (!history)
      history = [];

    const query = new Query(history);
    const self = query;
    const stats = query.stats;

    function show_stats(question, matched, size) {
      console.log(`\n${question} matched ${matched.length} from ${size}`);
      matched.forEach((entry) => {
        console.log(`ratio: ${entry.ratio}, uuid: ${entry.uuid}\non ${entry.role}: ${entry.on}\n`);
      });
      console.log(`median:  ${stats.median(matched)}`);
      [10, 25, 50, 70, 75].forEach((percentile) => {
        console.log(`${percentile}%:     ${stats.calc(matched, percentile / 100.0)}`);
      });
    }

    function ask(questions) {
      function doAsk(question) {
        if (!question || question.length === 0)
          return;

        let matched = query.select(question);

// todo: needs work...
        show_stats(question, matched, query.history.length);

        const pct = 70.0;
        matched = query.reduce(pct, matched);

        console.log(`\n\nAfter reducing by ${pct}%`);
        show_stats(question, matched, query.history.length);
      }

      if (!self.nx.isArray(questions))
        questions = [questions];
      else
        questions = [].concat(questions);
      while (questions.length)
        doAsk(questions.shift());
    }

    function quizLoop() {
      if (!process.stdin.isTTY)
        return ask(self.fs.readFileSync(process.stdin.fd).toString().split('\n'));

      const rl = self.readline.createInterface(process.stdin, process.stdout);

      console.log('Ready');
      rl.on('line', (question) => {
        question = question.trim();

        if (question.length <= 0 || question === 'quit')
          process.exit(0);

        ask(question);
        console.log('\n');
      });
    }

    if (argv.length === 0)
      return quizLoop();

    ask(argv);
  }
}


// exports.Query.Test(['chase'].concat(process.argv.slice(2)), 'history.json');
// exports.Query.Test(['movie'], 'history.json');
// exports.Query.Test([], 'history.json');
