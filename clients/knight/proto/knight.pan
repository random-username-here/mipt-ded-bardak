###
### knight:* -- Knight unit control
###

client knight:move(int8 dx, int8 dy);
client knight:attack(id whom);

server knight:tick();
server knight:hp(int32 val);
server knight:at(int32 x, int32 y);
server knight:root(int32 x, int32 y, id who);
server knight:wall(int32 x, int32 y);
