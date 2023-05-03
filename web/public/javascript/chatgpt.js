const ChatConfig = {
  "chat": {
    "debug": {
      "history": false,
      "protocol": false,
      "dialog": false,
      "context": false
    },
    "enabled": {
      "history": true,
      "usage": false,
      "save": {
        "history": false,
        "context": false
      }
    },
    "history": [],
    "dialog": {
      "user": "user",
      "system": "assistant",
      "model": "text-davinci-003",
      "temperature": 0,
      "top_p": 1,
      "max_tokens": 4000,
      "frequency_penalty": 0,
      "presence_penalty": 0,
      "stop": [
        "\"\"\""
      ]
    }
  },
  "proxy": {
    "debug": {
      "history": false,
      "protocol": false,
      "dialog": false,
      "context": false
    },
    "targetUrl": "https://api.openai.com",
    "proxyServerUrl": "http://localhost:3000/chat"
  }
}
