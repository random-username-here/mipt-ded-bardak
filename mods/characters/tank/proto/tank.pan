###
### tank:* -- Tank unit control
###

client tank:move();
client tank:rotate(int8 dir);
client tank:shoot();

server tank:tick();
server tank:hp(int32 val);
server tank:at(int32 x, int32 y);
server tank:root(int32 x, int32 y, id who);
server tank:enemy(int32 x, int32 y, id who, char64 kind);
server tank:wall(int32 x, int32 y);

