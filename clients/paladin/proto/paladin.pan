###
### paladin:* -- Paladin unit control
###

client paladin:move(int8 dx, int8 dy);
client paladin:use(string ability, id target);

server paladin:tick();
server paladin:hp(int32 val);
server paladin:at(int32 x, int32 y);
server paladin:root(int32 x, int32 y, id who);
server paladin:enemy(int32 x, int32 y, id who, char64 kind);
server paladin:wall(int32 x, int32 y);
server paladin:item(string id);
server paladin:ability(string id);
