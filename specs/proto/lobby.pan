###
### lobby:* -- Server-side match lobby
###

server lobby:queued(int32 required, int32 waiting);
server lobby:start(int32 match, int32 players);
server lobby:finish(int32 match, string result, id winner, string winnerRole);
