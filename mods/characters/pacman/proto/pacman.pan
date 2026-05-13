###
### pacman:* -- Pac-Man role control
###

client pacman:move(int8 dx, int8 dy);

server pacman:tick();
server pacman:hp(int32 val);
server pacman:at(int32 x, int32 y);
server pacman:sees(int32 x, int32 y, id who);
server pacman:wall(int32 x, int32 y);
