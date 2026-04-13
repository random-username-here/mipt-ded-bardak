###
### turns:* -- Turn-based strategy
###

# It is your turn
server turns:turn();

# Skip this turn
server turns:skip();

# You sent something, but this is not your turn
server turns:r.!your(id msg);
