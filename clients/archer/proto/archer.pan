###
### archer:* -- Archer unit control
###

client archer:move(int8 dx, int8 dy);
client archer:use(string ability, id target);

server archer:tick();
server archer:hp(int32 val);
server archer:at(int32 x, int32 y);
server archer:root(int32 x, int32 y, id who);
server archer:enemy(int32 x, int32 y, id who, char64 kind);
server archer:wall(int32 x, int32 y);
server archer:item(string id);
server archer:ability(string id);
