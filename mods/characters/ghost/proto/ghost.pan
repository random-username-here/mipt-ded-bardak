###
### ghost:* -- Ghost role control
###

client ghost:move(int8 dx, int8 dy);
client ghost:attack(id whom);
client ghost:where(id teamId);
client ghost:sees();

server ghost:tick();
server ghost:hp(int32 val);
server ghost:at(int32 x, int32 y);
server ghost:where(int32 x, int32 y, id who, id teamId);
server ghost:sees(int32 x, int32 y, id who, id teamId);
server ghost:wall(int32 x, int32 y);
