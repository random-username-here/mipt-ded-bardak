###
### ghost:* -- Ghost role control
###

client ghost:move(int8 dx, int8 dy);
client ghost:attack(id whom);
<<<<<<< HEAD
=======
client ghost:where(id teamId);
client ghost:sees();
>>>>>>> 584eaeb (feat: refactor client, add pacman script, add role manager (sevaphasol squash for rebasing))

server ghost:tick();
server ghost:hp(int32 val);
server ghost:at(int32 x, int32 y);
<<<<<<< HEAD
server ghost:sees(int32 x, int32 y, id who);
=======
server ghost:where(int32 x, int32 y, id who, id teamId);
server ghost:sees(int32 x, int32 y, id who, id teamId);
>>>>>>> 584eaeb (feat: refactor client, add pacman script, add role manager (sevaphasol squash for rebasing))
server ghost:wall(int32 x, int32 y);
