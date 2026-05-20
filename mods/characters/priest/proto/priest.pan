###
### priest:* -- Paladin unit control
###

client priest:move(int8 dx, int8 dy);
client priest:use(string ability, id target);

server priest:tick();
server priest:hp(int32 val);
server priest:at(int32 x, int32 y);
server priest:root(int32 x, int32 y, id who);
server priest:enemy(int32 x, int32 y, id who, char64 kind);
server priest:wall(int32 x, int32 y);
server priest:item(string id);
server priest:ability(string id);
