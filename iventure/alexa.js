var util = require('util');
var fs = require('fs');
var http = require('http');
var express = require('express');
var stringify = require('json-stringify-safe');


function log(text) {
    if (false) {
        var f = util.format.apply(this, arguments);
        console.log(new Date().toISOString().replace('T', ' ') + ': ' + f);
    } else
        console.log(text);
    return text;
}


function clone(o) {
    return JSON.parse(JSON.stringify(o));       // clone...
}


var alexaOutputSpeechResponse =
    {
        "version": "1.1",
        "sessionAttributes": {
            "key": "value"
        },
        "response": {
            "shouldEndSession": false,
            "outputSpeech": {
                "type": "PlainText",
                "text": "Plain text string to speak",
                "ssml": "<speak>SSML text string to speak</speak>",
                "playBehavior": "ENQUEUE"
            }
        }
    };


function sendResponse(rsp, status, text) {
    try{
    rsp.status = status;
    rsp.statusCode = status;
    rsp.write(text);
    if (status != 200)
        console.log(text);
    return rsp.end();
    } catch (e) {
        log( e.toString());
    }
}


var app = express();
var bodyParser = require('body-parser');
app.use(bodyParser.text({type: '*/*', limit: '1mb'}));

var requests = [];
var responses = [];

// set default page
app.post('/', function (req, rsp) {

        var val = '';
        var rjson = rjson = clone(alexaOutputSpeechResponse);

        try {
            val = JSON.parse(req.body);
            requests.push(stringify(val));

            log(val.request.type);

            switch (val.request.type) {
                default:
                    rjson.response.outputSpeech.text = log("Don't recognize: " + val.request.type);
                    break;

                case "FallbackIntent":
                    rjson.response.outputSpeech.text = "Fall Back";
                    break;

                case "CancelIntent":
                    rjson.response.outputSpeech.text = "Cancel";
                    break;

                case "HelpIntent":
                    rjson.response.outputSpeech.text = "Help";
                    break;

                case "StopIntent":
                    rjson.response.outputSpeech.text = "Stop";
                    break;

                case "NavigateHomeIntent":
                    rjson.response.outputSpeech.text = "Navigate";
                    break;

                case "move":
                    rjson.response.outputSpeech.text = "move";
                    break;

                case  "LaunchRequest":
                    rjson.response.outputSpeech.text = "So, you want to play a game!; Try look, go and talk";
                    break;

                case "SessionEndedRequest":
                    rjson.response.outputSpeech.text = "goodbye";
                    break;

                case "IntentRequest":
                    var command = val.request.intent.name;
                    var slots = val.request.intent.slots;
                    if (slots !== undefined) {
                        Object.keys(slots).forEach(function (key, index) {
                            var v = slots[key];
                            if (v.value !== undefined)
                                command += ' ' + v.value;
                        });
                    }
                    rjson.response.outputSpeech.text = doIventureCommand(command);
                    break;
            }
        } catch (e) {
            return sendResponse(rsp, 400, e.toString());
        }

        responses.push(stringify(rjson));
        return sendResponse(rsp, 200, stringify(rjson));
});


var httpServer = http.createServer(app);

httpServer.listen(5000);


var iventure = require('./iventure.js');

function doIventureCommand(command) {
    console.log(command);
    var text = iventure.doCommand(command);
    return text;
}
