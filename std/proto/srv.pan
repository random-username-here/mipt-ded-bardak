###
### srv:* -- Standard server messages
###
### Those messages are by server itself
###

# Server has given prefix availiable
# Sent at connection
server srv:hasPref(char64 pref);

# Server is named like that
# For displaying in UI, sent at connection
server srv:name(string name);

# This connection was given that ID
# Sent at connection
server srv:id(id clientId);

# This connection has given level
# ("admin"/"specator"/"player")
server srv:level(char64 level);

# Try changing level
client srv:setLvl(char64 level);
server srv:r.setLvl(id msg, bool ok);

# Set user's name
client srv:username(string name);
