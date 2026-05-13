###
### mage:* -- Mage unit control
###

client mage:move(int8 dx, int8 dy);
client mage:use(string ability, id target, int32 x, int32 y);

server mage:tick();
server mage:hp(int32 val);
server mage:at(int32 x, int32 y);
server mage:root(int32 x, int32 y, id who);
server mage:enemy(int32 x, int32 y, id who, char64 kind);
server mage:wall(int32 x, int32 y);
server mage:item(string id);
server mage:ability(string id);
