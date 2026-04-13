###
### gmap:* -- Square tile-based map
###

# Map data, sent on connection
# Data consists of 1 byte per tile -- tile type
# Currently there are 2 tile types:
#  - 0 : ground
#  - 1 : wall
server gmap:map(int16 width, int16 height, blob data);

# Move by (dx, dy) on the map
client gmap:move(int8 dx, int8 dy);
server gmap:r.move(id req, bool ok);

# Get information about visible units
client gmap:getVis();
server gmap:r.vis(id unitId, char64 unitType, int8 dx, int8 dy);
