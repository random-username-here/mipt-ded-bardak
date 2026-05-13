###
### rogue:* -- Rogue skeleton unit control
###

client rogue:move(int8 dx, int8 dy);
client rogue:use(string ability, id target);

server rogue:tick();
server rogue:hp(int32 val);
server rogue:at(int32 x, int32 y);
server rogue:root(int32 x, int32 y, id who);
server rogue:enemy(int32 x, int32 y, id who, char64 kind);
server rogue:wall(int32 x, int32 y);
server rogue:item(string id);
server rogue:ability(string id);
