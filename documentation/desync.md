# Desynchronizations

This document describes the various desync cases seen in Odamex, their causes, and what we do to address them.

## Entity "ghost" Desyncs

### Slaughter map ghosts

Prior to 13.0, entities would desync on the client due to out-of-order or skipped receipt of entity creation and kill or removal messages.
The higher the packet load, the more severe the problem became.  Slaughter maps were especially bad.

The fix was for clients to perform enforced, ordered processing of received reliable packets from the server and for the server to never
prematurely empty the 

### Sleepwalkers

Prior to 13.0, sector soundtargets would desync because there was no way for clients to recognize that the server considered other players to be soundtargets.
Clients could only ever set soundtargets to either the local player or null.

This ultimately created a situation where if one player died while being pursued by monsters, and an out-of-sight observing player had ever made a noise in
or near the sector where the death happened, the observer would see the monsters start chasing them while the server actually considers the monsters as
having gone to sleep.  If that observer starts making noise, moves to a location where the server considers them to be visible to the actually-idle
monsters, or anything else that causes the monsters to wake up, then the ghosts instantly teleport to their true location and start acting normal.

The fix was to add a `NoiseAlert` message that the server broadcasts to the clients whenever a player makes a noise that causes any sector's soundtarget
to change.  Upon receipt, the clients propagate the NoiseAlert for the given target, bringing the sector soundtargets into sync, and allowing the monsters
to locally go idle upon target death as they do on the server.

## Weapon Desyncs

### Weapon pickup:  Player angle precision

By design, player input commands do not dictate or request specific positions - they only command an angle and player-relative motion.
The server must calculate the player's position from those commands and the client must dead-reckon a predicted result of those commands.

Prior to 13.0, the client sent the player input message with reduced-precision angles, leading to minute position differences between client and server.
Analysis showed that this could come out to roughly multi-centimeter-level disagreements, leading to mis-predicted weapon pickups.

The fix was to send the player-commanded angles to the server with full precision.

### Weapon pickup:  Lag jitter and tic boundary crossings

### Animation / state:
