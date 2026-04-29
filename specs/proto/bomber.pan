###
### bomber:* -- Bomber role control
###

client bomber:move(int8 dx, int8 dy);
client bomber:bomb();

server bomber:tick();
server bomber:hp(int32 val);
server bomber:at(int32 x, int32 y);
server bomber:sees(int32 x, int32 y, id who);
server bomber:wall(int32 x, int32 y);
