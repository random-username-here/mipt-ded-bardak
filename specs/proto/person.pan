###
### person:* -- Unit control
###

# +/- 1
client person:move(int8 dx, int8 dy);
client person:attack(id whom);

server person:tick();
server person:hp(int32 val);
server person:at(int32 x, int32 y);
server person:sees(int32 x, int32 y, id who);
server person:wall(int32 x, int32 y);
