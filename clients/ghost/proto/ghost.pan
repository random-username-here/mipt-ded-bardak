###
### ghost:* -- Ghost role control
###

client ghost:move(int8 dx, int8 dy);
client ghost:attack(id whom);

server ghost:tick();
server ghost:hp(int32 val);
server ghost:at(int32 x, int32 y);
server ghost:sees(int32 x, int32 y, id who);
server ghost:wall(int32 x, int32 y);
