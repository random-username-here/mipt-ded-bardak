###
### srv:* -- server utility messages
###

# --- Server hello

# Server's name (to be drawn in hud)
server srv:name(string name); 

# Your client id
server srv:id(id id); 

# Given prefix is availiable, end is at pref = 0
server srv:hasPref(char64 pref);

# --- Time correction (via udp)

client srv:tLocal(int64 client_ns);
server srv:tCorrect(int64 client_ns, int64 server_ns);


