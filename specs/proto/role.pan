###
### role:* -- Client role selection
###

client role:choose(string id);

server role:option(string id, string name, string prefix);
server role:chosen(string id);
server role:reject(string reason);
