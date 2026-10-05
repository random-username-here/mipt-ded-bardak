###
### syncv:* -- Syncvars
###

# TCP, notify clients about new syncvar
# Type: 'd' for double, 'i' for int64
server syncv:declare(id scope, id var, int8 type, string name);

# UDP, update variable
# doubles get transfered like int64-s, with cast on both sides
# (since we cannot have unions for now in this pan)
server syncv:update(id scope, id var, int64 time, int64 val);

# TCP, the syncvar was deleted
server syncv:remove(id scope, id var, string name);
