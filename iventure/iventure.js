/**
 * Created by hbray on 2/25/2016.
 */
const util = require('util'),
    fs = require('fs'),
    stringify = require('json-stringify-safe'),
    readline = require('readline'),
    nx = require('@zality/nxmesh');


const Requirement = new Enum({
    approximate: 2,
    required: 1,
    notRequired: 0
});


let Game = undefined;

function log(text) {
    if (false) {
        const f = util.format.apply(this, arguments);
        console.log(new Date().toISOString().replace('T', ' ') + ': ' + f);
    } else {
        if (isObject(text))
            text = stringify(text);
        console.log(text);
    }

    return text;
}

function fatal(message) {
    throw new Error(log(message.toString()));
    process.exit(1);
}


/**
 * @return {boolean}
 */
function isNull(it, isRequired) {
    if (isRequired === undefined || isRequired === null)
        isRequired = Requirement.vals.notRequired;
    if (it === undefined || it === null) {
        if (!isRequired)
            return true;
        fatal(stringify(it) + ' must not be undefined');
    }
    return false;
}

/**
 * @return {boolean}
 */
function isObject(it, isRequired) {
    if (isNull(isRequired))
        isRequired = Requirement.vals.notRequired;
    if (!isNull(it) && it.constructor === Object)
        return true;
    if (!isRequired)
        return false;
    fatal(stringify(it) + ' must be an Object');
}


function isFunction(it, isRequired) {
    if (isNull(isRequired))
        isRequired = Requirement.vals.notRequired;

    if (typeof it === 'function')
        return true;
    if (!isRequired)
        return false;
    fatal(stringify(it) + ' must be a Function');
}


/**
 * @return {boolean}
 */
function isArray(it, isRequired) {
    if (isNull(isRequired))
        isRequired = Requirement.vals.notRequired;
    if (it instanceof Array)
        return true;
    if (!isRequired)
        return false;
    fatal(stringify(it) + ' must be an Array');
}


/**
 * @return {boolean}
 */
function isString(it, isRequired) {
    if (isNull(isRequired))
        isRequired = Requirement.vals.notRequired;
    if (typeof (it) === 'string')
        return true;
    if (!isRequired)
        return false;
    fatal(stringify(it) + ' must be a String');
}

/**
 * @return {boolean}
 */
function isTrue(it, isRequired) {
    if (isNull(isRequired))
        isRequired = Requirement.vals.notRequired;

    if (it === undefined && isRequired)
        fatal(stringify(it) + ' must be Present');

    if (it !== undefined && it)
        return true;
    return false;
}


function textOfArray(t) {
    if (isArray(t))
        return t.join(' ');
    return t;
}


/**
 * @return {void}
 */
function iterateGameObject(gameObj, cb) {
    function iterator(iobj, cb, iparent, ikey) {
        if (isArray(iobj)) {
            const al = iobj.length;
            for (let i = 0; i < al; ++i) {
                iterator(iobj[i], cb, iobj, i);
            }
        } else if (isObject(iobj)) {
            if ( (!isString(ikey)) || (!ikey.startsWith('_'))) {
                if (isFunction(cb))
                    cb(iobj, iparent, ikey);
                Object.keys(iobj).forEach(function (key) {
                    iterator(iobj[key], cb, iobj, key);
                });
            }
        } else {
            if ( (!isString(ikey)) || (!ikey.startsWith('_'))) {
                if (isFunction(cb))
                    cb(iobj, iparent, ikey);
            }
        }
    }

    iterator(gameObj, cb);
}


function verifyGame(game) {
    isObject(game, Requirement.vals.required);

    iterateGameObject(game, function (iobj) {
        if (isObject(iobj)) {
            if (isNull(iobj._name))
                fatal(stringify(iobj) + ' does not have a name');
            if (isNull(iobj._path))
                fatal(stringify(iobj) + ' does not have a path');
        }
    });
}


function cleanGame(game) {
    isObject(game, Requirement.vals.required);

    iterateGameObject(game, function (iobj, iparent, ikey) {
        if (isObject(iobj)) {
            delete iobj['_name'];
            delete iobj['_path'];
        }
    });
}


function applyGameTags(game) {
    isObject(game, Requirement.vals.required);

    iterateGameObject(game, function (iobj, iparent, ikey) {
        if (isObject(iobj)) {
            iobj._name = isNull(ikey) ? 'game' : ikey.toString().trim();
            iobj._path = (isNull(iparent) ? '' : iparent._path) + '/' + iobj._name;
        }
    });
}

function loadGameFile(fileName) {
    const game = {};

    try {
        game.fileName = fileName;
        game.src = fs.readFileSync(fileName).toString();
        game.obj = JSON.parse(game.src);
    } catch (ex) {
        log(ex.toString());
        log(fileName + ' contains an invalid game');
        process.exit(1);
    }

    cleanGame(game.obj);
    return game;
}

function loadGame(fileName) {
    const game = loadGameFile(fileName);

    try {
        applyGameTags(game.obj);
        verifyGame(game.obj);
    } catch (ex) {
        log(ex.toString());
        log(fileName + ' contains an invalid game');
        process.exit(1);
    }

    return game;
}


function getGameValue(from, path, isRequired) {
    if (isNull(isRequired))
        isRequired = Requirement.vals.notRequired;

    isObject(from, Requirement.vals.required);
    isString(path, Requirement.vals.required);

    if (path[0] === '/')	// absolute reference
        from = Game.obj;		// start looking from the top

    // split the path
    const keys = path.toString().trim().split('/');

    // look for the gameObj
    let gameObj = from;
    const al = keys.length;
    for (let i = 0; i < al; ++i) {
        const key = keys[i];
        if (key.length > 0) {
            if (!(gameObj = gameObj[key]))
                break;      // not found
        }
    }

    if (isNull(gameObj) && isRequired)
        fatal('Unable to find ' + path);

    return gameObj;
}


function setGameValue(from, path, value) {
    isNull(value, Requirement.vals.required);
    const ov = getGameValue(from, path, Requirement.vals.isRequired);
    if (isObject(ov)) {
        say("You can't modify " + path);
        return undefined;
    }
    const rindex = path.lastIndexOf('/');
    const prop = path.substring(rindex + 1);	// get the property name being modified
    path = path.substring(0, rindex);
    // then get the gameObj
    const vobj = getGameValue(from, path, Requirement.vals.isRequired);
    vobj[prop] = value;			// set the new value

    return ov;
}


const sayBuffer = [];

function say(text) {
    text = textOfArray(text);
    console.log(text);
    sayBuffer.push(text);
    return text;
}


function clone(o) {
    return JSON.parse(JSON.stringify(o));		// clone...
}


/**
 * @return {string}
 */
function removeNoiseWords(input) {
    const noise = {
        'i': true,
        'a': true,
        'an': true,
        'at': true,
        'do': true,
        'is': true,
        'the': true,
        'to': true,
        'too': true,
        '': false		// place holder
    };

    const iwords = input.toString().trim().split(' ');
    const owords = [];
    const al = iwords.length;
    for (let i = 0; i < al; ++i) {
        const word = iwords[i];
        if (!noise[word])
            owords.push(word);
    }
    return owords.join(' ');
}


/**
 * @return {object}
 */
function getGamePlayerObject() {
    return getGameValue(Game.obj, '/actors/player', Requirement.vals.required);
}


/**
 * @return {object}
 */
function getPlayersValue(path) {
    return getGameValue(getGamePlayerObject(), path, Requirement.vals.required);
}


//
// Requires:
//	command: applies to given object: look/go left...
//
// Returns: string[]
//

function getPlayerActions(command) {
    const actlist = [];

    function add(cmd, act) {
        const a = [cmd];
		// commands will always be an array
        if (isArray(act))
            a.push(act);
        else
            a.push([act]);
        actlist.push(a);
    }

    function addActions(actobj) {
        Object.keys(actobj).forEach(function (ikey) {
            if (ikey.startsWith('_'))
                return;
            const action = actobj[ikey];
            const cmd = [ikey];
            if (isObject(action)) {
                Object.keys(action).forEach(function (key) {
                    if (key.startsWith('_'))
                        return;
                    const nc = [key, cmd[0]];
                    add(nc, action[key]);
                });
            } else {
                add(cmd, action);
            }
        });
    }

    const actions = {};
    actions.command = command;
    actions.parsed = removeNoiseWords(command);			// remove common english articles: at, the, etc...
    actions.pobj = getGamePlayerObject();
    actions.location = getGameValue(actions.pobj, 'location');
    actions.lobj = getGameValue(Game.obj, actions.location);

    addActions(getGameValue(actions.pobj, 'actions')); // player actions
    addActions(getGameValue(actions.lobj, 'actions')); // actions in player location
    actions.actlist = actlist;
    return actions;
}


/**
 * @return {string}
 */
function resolveRef(from, text) {
    const iwords = text.toString().trim().split(' ');
    const owords = [];
    for (let w = 0, al = iwords.length; w < al; ++w) {
        let word = iwords[w];

        if (word[0] === '~') {
            word = word.substring(1).trim();
            const en = getGameValue(from, word);
            if (isString(en) || isArray(en))
                owords.push(textOfArray(en));
            else
                owords.push(word);
        } else
            owords.push(word);
    }
    return owords.join(' ');
}


//
// Requires: object with: {command: string, parsed: string, location: string, lobj: object, lacts: object, pacts: object}
// Built by: getPlayerActions()
//
function doActions(actobj) {

    function doAction(action) {
        let didit = false;
        let exp = action.toString().trim().split(':');
        let lhs = exp[0];

        exp.shift();
        let rhs = exp.join(':');

        switch (lhs) {
            case 'say':
                say(resolveRef(actobj.lobj, rhs.trim()).trim());
                didit = true;
                break;

            case 'set':
                exp = rhs.toString().trim().split('=');
                lhs = exp[0];
                rhs = exp[1];
                const add = lhs.endsWith('+');
                if (add)
                    lhs = lhs.slice(0, lhs.length - 1);		// remove '+' symbol

                let t = getGameValue(Game.obj, lhs);
                if (isNull(t)) {
                    say("There's no " + lhs + ' here');
                    break;
                }
                t = setGameValue(Game.obj, lhs, rhs);
                if (lhs === '/actors/player/location')
                    doUserCommand('look');		// updated scene, take a look around
                didit = true;
                break;

            default:
                // doCommand(action.toString());
                break;
        }

        return didit;
    }


// try the actions
    function iterateActions(actlist, cb) {
        isFunction(cb, Requirement.vals.required);

        let didit = false;
        const al = actlist.length;
        for (let i = 0; i < al; ++i) {
            if ((didit = cb(actlist[i])))
                break;
        }

        return didit;
    }

    const parsed = actobj.parsed.trim();

    let didit = false;
    if (parsed === 'help') {
        const help = [];
        didit = iterateActions(actobj.actlist, function (action) {
            help.push(textOfArray(action[0]).trim());
            return false;		// signal get more...
        });
        help.sort();
        say('Try:');
        const al = help.length;
        for (let i = 0; i < al; ++i) {
            say('  ' + help[i]);
        }
        didit = true;
    }

    if (!didit) {
        didit = iterateActions(actobj.actlist,
            function (action) {
                if( parsed === textOfArray(action[0]).trim()) {
        			const al = action[1].length;
        			for (let i = 0; i < al; ++i) {
            			didit |= doAction(action[1][i]);
        			}
				}
                return didit;
            });
    }

    return didit;
}


function doUserCommand(command) {
    if (command === 'restart') {
        Game = loadGame(Game.fileName);
        return;
    }

    if (command === '?') 		// request for help
        command = 'help';

    const actions = getPlayerActions(command);
    const didit = doActions(actions);
    if (!didit)
        say("that can't be done here");

    // if its a look; show items in room and players possessions
    if (actions.parsed === 'look') {
        // showThings(actions.from.props, 'You see');
        // showThings(getPlayersValue('props'), 'You have');
    }
}


const debugLocation = '/';

function doDebugCommand(command) {

    command = command.toString().toLowerCase().trim();

    let it;
    if (command === '*')
        it = Game.obj;
    else
        it = getGameValue(Game.obj.actors.player, command);

    if (!isNull(it)) {
        say(stringify(it, null, 4));
    } else {
        say(command + ': not found\n');
    }
}


function doCommand(command) {
    command = command.toString().toLowerCase().trim();

    if (command === 'quit')
        process.exit(0);

    // iterate over multiple commands separated by ;
    if (command.indexOf(';') >= 0) {
        const lines = command.toString().trim().split(';');
        const al = lines.length;
        for (let i = 0; i < al; ++i)
            doCommand(lines[i]);
        return;
    }

    if (command.length <= 0)
        return;

    try {
        if (command.startsWith('.')) {
            doDebugCommand(command.slice(1));
        } else {
            doUserCommand(command.toString());
        }
    } catch (ex) {
        log(ex.toString());
        log(ex.stack);
        process.exit(1);
    }
}


/**
 * @return {string}
 */
function doIventureCommand(command) {
    sayBuffer.length = 0;
    doCommand(command);
    return sayBuffer.join(' ');
}


/****************************************************************************************************
 Start Here
 ****************************************************************************************************/

function run() {
    const stdin = process.stdin;
    const stdout = process.stdout;

    const rl = readline.createInterface(stdin, stdout);

    Game = loadGame('game.json', false);

// check for run options
    const argv = [];
    for (let i = 2; i < process.argv.length; ++i) {
        const option = process.argv[i].toString().trim();
        switch (option) {
            case '-d':
                cleanGame(Game.obj);
                console.log(stringify(Game.obj, null, 4));
                process.exit(0);
                break;
            default:
                argv.push(option);
                break;
        }
    }

// verify we can find the player...
    const player = getGamePlayerObject();
// and its location
    const location = getPlayersValue('location');

    if (argv.length > 0) {
        for (let i = 0; i < argv.length; ++i) {
            const command = argv[i].toString().trim();
            if (command.length > 0) {
                say(command);
                doCommand(command);
            }
        }
    } else {
        doCommand('look');
    }

    rl.on('line', (line) => {
        line = line.toString().trim().toLowerCase().trim();
        doCommand(line);
    });
}


function test() {
    Game = loadGame('game.json');
    process.exit(0);
}


run();

module.exports = {run: run, test: test, doCommand: doIventureCommand};
