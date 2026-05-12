###
### pacman:* -- Pac-Man role control
###

client pacman:move(int8 dx, int8 dy);
client pacman:where(id teamId);
client pacman:sees();

server pacman:tick();
server pacman:hp(int32 val);
server pacman:at(int32 x, int32 y);
server pacman:where(int32 x, int32 y, id who, id teamId);
server pacman:sees(int32 x, int32 y, id who, id teamId);
server pacman:wall(int32 x, int32 y);
