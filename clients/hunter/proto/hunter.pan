###
### hunter:* -- Hunter unit control
###

client hunter:move(int8 dx, int8 dy);
client hunter:use(string ability, id target);

server hunter:tick();
server hunter:hp(int32 val);
server hunter:at(int32 x, int32 y);
server hunter:root(int32 x, int32 y, id who);
server hunter:enemy(int32 x, int32 y, id who, char64 kind);
server hunter:wall(int32 x, int32 y);
server hunter:item(string id);
server hunter:ability(string id);
