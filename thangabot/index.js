const login = require("facebook-chat-api");

login({email: "redacted", password: "readcted"}, (err, api) => {
    if(err) return console.error(err);

    api.setOptions({listenEvents: true});

    api.listen((err, event) => {
        if(event.logMessageType == 'log:user-nickname') {
            api.changeNickname('',event.threadID,event.logMessageData.participant_id, (err) => {
                if(err) return console.error(err);
            });
        }
    });
});

