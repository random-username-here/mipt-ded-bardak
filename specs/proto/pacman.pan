###
### pacman:* -- Pac-Man role control
###

client pacman:move(int8 dx, int8 dy);
<<<<<<< HEAD
=======
client pacman:where(id teamId);
client pacman:sees();
>>>>>>> 584eaeb (feat: refactor client, add pacman script, add role manager (sevaphasol squash for rebasing))

server pacman:tick();
server pacman:hp(int32 val);
server pacman:at(int32 x, int32 y);
<<<<<<< HEAD
server pacman:sees(int32 x, int32 y, id who);
=======
server pacman:where(int32 x, int32 y, id who, id teamId);
server pacman:sees(int32 x, int32 y, id who, id teamId);
>>>>>>> 584eaeb (feat: refactor client, add pacman script, add role manager (sevaphasol squash for rebasing))
server pacman:wall(int32 x, int32 y);
