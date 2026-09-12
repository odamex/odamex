---
marp: false
theme: odamex1
size: 16:9
paginate: true

header: ' '

---

<!-- _class: lead -->

# Odamex 2026 netcode refactor

Jim Thoenen

---

# What problem are we solving?

## Catastrophic desynchronizations under load

- Entities (monsters, missiles, special effects, et al)
  - aka "**Ghosts**"
- Player state (health, armor, weapons, ammo, et al)
  - aka "**Weapon desync**"
- Map automations via avatar/voodoo doll (sector motion, specials and line activation)
  - aka "**Map doesn't work in multiplayer**"

As a player, I want to play large maps with very high monster counts and complex map
automations with my friends, so that we can enjoy the latest MBF21 wads together.

---

# What is the proposed solution?

## Industrial-grade enhancements to netcode:

#### **Reliability** refactor
#### New **prioritization and throttling** behaviors
#### **Rollback-based deconfliction** for multiple player attributes

---

# Assumptions and Constraints

## The PvP experience must not be adversely impacted

Odamex has a key role in competitive play.  Genuine improvements in PvP are welcome,
but the experience must not degrade.

## The server is *always* the canonical source-of-truth

The client is allowed to predict and dead-reckon items that can desync, but they must
resync and rollback if necessary once a conflicting update arrives from the server.

---

# Assumptions and Constraints - cont'd

## Predict elements that prioritze fast-action movement and the trigger-pull experience

<div class="two-columns">
<div>

#### Predicted client-side:

- Player movement
- Player-initiated hitscans (aka unlagged)
- Weapon switch
- Psprites
- Weapon pickup
- Ammo expenditure
- Sector motion

</div>
<div>

#### **Not** predicted:

- Any *NON*-weapon pickup
- Mobj spawns, removals (monsters, weapons, effects, explosions, etc)
  - Prematurely showing or hiding these things is *very* detrimental to the PvP experience
  - Special exception for hitscan bullet puffs, bloodshed

</div>
</div>

---

# Key components of the solution

## Transports: Reliable, Best-Effort, and High-Priority Best-Effort

Implemented by new `OdaMessenger` class

## Prioritization and Throttling

Implemented in server-side mobj "awareness" and client-side mobj "credibility"

## Rollbacks

Implemented in server-side attribute "monitors" and client-side "roller states"

---

<!-- _class: bumper -->

# Desyncs: Ghosts and glitches, good grief

---

# `OdaMessenger` class

New infrastructure to manage protocol-level message transport

Key responsibilities:

- Establishing Reliable, Best-Effort, and High-Priority Best-Effort transports
- Reliable packet sequencing, ack, and retransmission:  `SequenceSender` class and `SequenceReceiver` class
- Marshalling and unmarshalling of messages into/out of packets
- Capturing performance and bandwidth metrics

---

# `OdaMessenger` - Reliable transport

Intended for event-like data that cannot be missed:
- **Late reception is always preferable to no-reception**

Guaranteed packet delivery and order
- Enforced sequence-ordering of packets on both transmission and reception
- Receiver acks, sender retransmits if ack not seen by deadline

Server: The ack deadline is derived from ping time plus error margin
- In a high-ping/low-bandwidth situation, early retransmissions are more detrimental than late retransmissions

Client: The ack deadline is *zero* - more on this in a bit

Variety of APIs to tune behaviors

---

# `OdaMessenger` - Best-effort transport

Intended for frequently-updated, state-like data that are *No Big Deal* to miss

Sequenced, but only enough to indicate concurrency with reliable packets and discard data that are known-supersceded

Ideal for predictable, non-high-priority mobj position, momentum, etc updates

Fundamentally lower-priority than any Reliable or High-priority packets

---

# `OdaMessenger` - High-priority, Best-effort transport

For frequently-updated, state-like data that, if missed, is detrimental but not catastrophic

Sequenced like ordinary Best-effort, but higher priority than Reliable

Intended for data that is key to keeping the fundamental game running smoothly:
- Reliable acks
- Server gametic
- Player and Avatar mobj and state updates
- Mobj updates for weapons fired by players
- Sector updates

---

# Client-side immediate ack timeout.. why?

This causes the newest PlayerInput message to go out with packet-redundancy
- `PlayerInput` becomes tolerant to 1 client-to-server packet drop per tic

The number of historical redundant `PlayerInput` instances becomes naturally limited to what the bandwidth actually needs
- This was previously hardcoded to 10 `PlayerInput` (ticcmd) instances per packet

---

# Low-level prioritization

<div class="two-columns">
<div>

## Rate budget

A per-client, per-tic budget is calculated based on tunable rate

- Server-side: `sv_maxrate`
- Client-side: Fixed at 10 KBps

## Per-tic transmission order

1. High-priority packets
1. Reliable packets
   - including retransmissions
1. Best-effort packets

</div>
<div>

## Overload determination

Reliable packets restricted to 90% of the budget

- 10% is reserved as "breathing room" for high-priority and best-effort
- If enqueued Reliable packets would exceed 90%, the messenger is considered overloaded
  - Put a pin in that for throttle trigger...

</div>
</div>

---

# High-level prioritization

What messages "go first" into their respective packet types?

General priority is:

1. Global: Local and remote player states and events
1. Global: World states and events
1. Per-player: Near mobjs
1. Per-player: Far mobjs

The farther a mobj is from a player, the lower its priority

---

# High-level server-side prioritization

## Relative distance-based sort

For each client, mobjs are sorted into 3 high-level groups:

| group | relative distance partition |
| ----- | --------------------------- |
| Nearest | [0 - 25%) |
| Mid-range | [25% - 50%) |
| Distant | [50% - remaining] |

There is no absolute unit constraint on this grouping

---

# High-level server-side prioritization - cont'd

## Sort key is distance-squared from player to mobj
- For sorting purposes, distance-squared works as well as true distance
- 22k mobj sort as measured on development system:
  - 200 usec with `P_AproxDistance2`
  - 170 usec with `mobjInfo.distanceSquared = dx*dx + dy*dy;`
    - Tweaks to improve liklihood of SIMD vectorization are possible, if needed

## Using 2x `std::nth_element` partial sorts instead of `std::sort`
- We don't need an exact sort, just 2 partitions to create 3 groups
  ```c++
  std::nth_element(player.sortedMobjs.begin(), partitions.outerBoundary, player.sortedMobjs.end(), distanceCompare);
  std::nth_element(player.sortedMobjs.begin(), partitions.innerBoundary, partitions.outerBoundary, distanceCompare);
  ```
  - 40-70 usec for 2x `std::nth_element`, time complexity: N
  - 600-700 usec for 1x `std::sort`, time complexity: N log(N)

---

# An aside on performance - multithreading

## New cvar: **`net_maxthreads`**

The sort, message generation, and packet-send operations are performed for all clients at the same time, parallelized one client at a time across a configurable number of threads

- Default: `0` - thread count is based on the core count as reported by the OS

Even though the threads spend the vast majority of their time sleeping during typical load, it is **highly recommended** to consider how many servers you intend to run on the same machine, the number of cores the machine has, and the max number of players per server, and configure accordingly.

PROTIP: Slaughter maps will be the longest pole in the tent, so to speak

---

# High-level server-side prioritization - cont'd

## Awareness policy

Every mobj has a distinct and independent awareness level for every client.

<small><small>

|  Level | Policy | Spawn? |
| --------------- | ------ | ---- |
| `NOT_AWARE` | This mobj does not exist on the client - it is completely invisible and unknown | No |
| `FULLY_AWARE` | The client has this mobj and is receiving normal updates for it | Yes |
| `SEMI_AWARE` | The client has this mobj but *unnecessary* reliable updates are sent best effort | Yes |
| `BARELY_AWARE` | This mobj is on "life support" for the client and is dead-reckoning only | No |
| `ALWAYS_AWARE` | This mobj is permanently Fully Aware.  It cannot be downgraded | Yes |

</small></small>

---

# High-level server-side prioritization - cont'd

General:
- Child mobjs inherit their parents' awareness
- Players and Avatars start off `ALWAYS_AWARE`, everything else starts off `NOT_AWARE`

`SEMI_AWARE`:
- "Unnecessary" reliable updates meaning ultimately cosmetic updates (such as **ExplodeMobj**) become best-effort

`BARELY_AWARE`:
- Server stops sending scheduled mobj update messages to the client
- Nothing can spawn-in as `BARELY_AWARE`, parent or child, it can only be downgraded into that mode
  - e.g. such a monster's missile shot is `NOT_AWARE` and stays that way until spawned-in mid-flight due to promotion
- However, it can be removed or killed

---

# High-level server-side prioritization - cont'd

## Special case for MF_MISSILE: **Hyper-aware**

Missiles typically have low update rates - Once every:
- 30 tics for most missiles,
- 7 tics for seekers and manc fireballs

When any blockmap missile comes within 64 units of a player, the player receives an update for that missile every tic.
- 64 units: half the width of the MAP01 initial hallway

Prevents incoming predicted missles from "hovering in the player's face" desync

---

# High-level server-side prioritization - cont'd

<div class="two-columns">
<div>

## Throttle

If the messenger does not indicate overload, then all distance groups are `FULLY_AWARE`.
- Ideal playing scenario

If the messenger indicates overload, then the distance-sorted groups are assigned:
- Nearest: `FULLY_AWARE`
- Mid-range: `SEMI_AWARE`
- Distant: `BARELY_AWARE`

</div>
<div>

## Throttle control heuristic

If the messenger continues to be overloaded, then the boundaries are
brought "closer-in" by powers of two until overload condition clears.

If the messenger continues to be NOT overloaded, then the boundaries are
pushed "further-out" until overload reappears or the throttle disengages

</div>
</div>

---

# The haunted tradeoff: "That's a big twinkie"
<style scoped>
h1
{
    margin-bottom: 5;
    padding-bottom: 0;
}
h4
{
    margin-top: 0;
    padding-top: 0;
}
</style>


#### The good: **Huge slaughterfests**
- Server-side prioritization and throttle allow very large crowds of mobjs to work with heavy loads
and relatively constrained bandwidth
- Resyncing happens naturally as players and mobjs gradually close their distance, often out-of-sight
because there are closer, more-aware mobjs monopolizing players' attention

#### The bad: **Ghosts**
- Very distant, `BARELY_AWARE` monsters are allowed to coast on dead-reckoning for lengthy periods
- They can diverge *substantially* from their canonical state on the server
- The farther afield, the less likely the server will automatically resync it as the player approaches:
  it has no idea where the client-side ghost wandered - it may still sort as "Distant"

Players teleporting across a map while throttled may find themselves ***in spook central***
#### **Who you gonna call?**

---

# "Ghostbuster" desync mitigation: Credibility

## Credibility levels

To combat this, the client introduces its own concept of **Mobj Credibility**:
**The longer a mobj goes without a canonical update, the less-credible it becomes**

<style scoped>
table
{
    font-size: 70%;
}
</style>

|  Level | Policy |
| --------------- | ------ |
| `FULLY_CREDIBLE` | The mobj is fresh and receives frequent updates. **Is allowed to trigger specials from prediction** |
| `ALWAYS_CREDIBLE` | This mobj is permanently Fully Credible.  Reserved for Avatars |
| `SEMI_CREDIBLE` | The mobj is at least 3 tics overdue for an update.  Cannot trigger specials from prediction |
| `NOT_CREDIBLE` | This mobj is at least 10 seconds overdue for an update.  Becomes eligible for a challenge |
| `CHALLENGED` | The client polls the server for a one-shot update on this mobj  |

---

# Ghostbusting: Credibility challenge

`NOT_CREDIBLE` monsters are transitioned to `CHALLENGED` based on client draw-order: **nearest-first**

The challenge is answered with an out-of-cycle, on-demand **UpdateMobj** sent reliably

Only one challenge allowed per tic:  35 ghosts busted per second

Anecdote: The only way I've been able to reliably witness this in action is by setting
`sv_maxrate` to 200, playing nuts.wad, getting blasted into a far corner by Archviles,
getting killed, and then respawning back at the spawn point.

---

<!-- _class: bumper -->

# Desyncs: Weapons and inventory

---

# Client-side player attribute rollbacks

"Weapon desyncs" originate from client-side mispredictions, most directly:

- Hitscan weapon trigger pulls predict changes to **ammo** and **psprites**
- Mispredicted edge-to-edge contact between players and **weapon pickups**

If we are going to do any client-side predictions at all, these are unavoidable

The task becomes to resync ASAP

---

# Resync strategy:

The server sends one of several tic-tagged messages that indicates accurate state
as of a given client-side tic:

- **PlayerInfo**: All key player attributes
- Fine-grained attribute messages (subset of above):
  - **PlayerAmmo**
  - **PlayerMaxAmmo**
  - **PlayerAmmo**
  - **PlayerWeaponOwned**
  - **PlayerWeaponSelection**
  - **PlayerPowers**
  - **PlayerPsprites**

Each of the above can be *independently mispredicted* by the client based on things
it is allowed to predict

---

# Attribute monitors

The server introduces attribute monitors for player data - anything that can be
independently mispredicted on the client side:

<div class="two-columns">
<div>

#### `LatchedItemMonitor`

- Pending weapon
- Ready weapon

</div>
<div>

#### `LatchedItemArrayMonitor`

- Weapons owned
- Ammo counts
- Maxammo
- Power counters
- Psprites

</div>
</div>

Monitors are armed, checked at end-of-tic, and fire off messages if their respective
attributes ultimately have different values at end vs start of tic.

---

# Roller states

The **attribute messages** and **PlayerInfo** are all tagged with the destination clients'
tic-of-validity as a natural time coordinate

- **Tic-of-validity**: The client's tic indicated in the PlayerInput that was effective as of the servertic that resulted in the changed attributes

Clients maintain "roller states" of every attribute encompassed in **PlayerInfo** so that
comprehensive rollbacks can be performed

1. Client receives a tic-tagged inventory message.
1. Client looks back at the history as of the tic-of-validity.
1. Client resolves and adjusts history in accordance with the message.
1. Client propagates the change in history forward to the current state.

---

# Roller states: resolution strategies

<div class="two-columns">
<div>

## Absolute

Attributes that don't have meaningful notion of delta, e.g. enumerated, boolean

- Weapon selection
- Armor type
- Weapon owned
- et al

</div>
<div>

## Relative / delta

Attributes that are quantities that have mathematical deltas

- Ammo
- Health
- Armor
- Lives
- et al

</div>
</div>

---

# Roller states: Psprite special

<style scoped>
p
{
    font-size: 80%;
}
</style>

Psprites are a special case due to functional and transitional behavior

If a psprite needs reconciliation, the "delta" is propagated based on whether the states "flow naturally" or if something unpredictable actually happened, e.g. player pulls trigger

## Next state determination

For a given psprite state, a "passive next state" can be calculated: decrement tics, select nextstate if tics == 0.

## Evaluation: reconcilation

1. For both the historical and corrected states, determine Next State.
1. Change historical sample to corrected state.
1. If next historical sample matches the calculated historical Next State, advance and repeat, else bail out.

---

<!-- _class: bumper -->

# Desyncs: Avatars and sectors / map automation

---

# Avatars

Avatars have existed for a while, and only needed some adjustment to fix map automation / sector / special effect desyncs

## Initial map automation desync

Some maps need tic 0 voodoo doll-driven activations, but a combination of factors prevented that from working

The solution:
1. clients **spawn Avatars immediately on map load**, and
1. the server sends a **ConfigureAvatar** that specifies its netid, keyed on map-defined spawnpoint

---

# Avatars cont'd

## Avatar priority

Avatars are treated with almost the same priority as actual players.

- Their updates come out in High priority packets
- Their priority is `ALWAYS_AWARE`, regardless of how physically far away they are from the client

## Misc fixes

A handful of scattered player / voodoo checks in the code needed adjustment to also apply to Avatars

---

# Sectors

Currently sector control messages are snapshot-oriented, and a few factors independently led to desyncs:

- Lack of reliability
- Some motion types had order-of-handling issues

## Fixes

- Send snapshots best-effort, except for the final end-state update, which is now sent Reliable
- Ensure deferred destruction of all sector mover thinkers

---

<!-- _class: bumper -->

# Miscellaneous netcode updates

---

# Misc

## Client-originated messages are now protobuf-ized

## **NetDemoCap** now captures all client-outbound messages

## The `svc_t` and `clc_t` are merged to `msg_t`

- The enumerals still identify as `clc_` or `svc_`, but their numeric values no longer collide
- Analyzing decompressed packets no longer requires special knowledge of packet origin

## `cl_netgraph` shows reliable metrics: bidir packets-in-flight, server throttle strength

---

# Miscellaneous Debris

## Cvars `net_sndbuf` and `net_rcvbuf` are removed

The network buffers are hardcoded at 128 KB now.  Allowing them to be configurable has done nothing but cause trouble

## Also, enable `r_drawnetcredibility` to visualize mobj credibility
---

<!-- _class: bumper -->

# Next steps

---

# What's next?

## More playtesting, more fixes - ongoing

It never stops, nor should it.

## Protocol hardening

There are vulnerabilities, opportunities for bad actors to ruin everyone's entire day.  These need to be fixed.

## Autocoding enum .proto file for hand-coded enums

- Build-time python script via CMake `custom_command`, ideally using libclang if the team is okay with that requirement
- Would REALLY help with netdemo analysis

---

# What else?

## Mathematical sector motion definition

Current sector motion messaging is overkill - Only need Reliable updates on accel/decel

- Start tic
- Start position
- End position
- Velocity
- Plus sector-specific behavior parameters relevant for prediction

Enough info for clients to dead-reckon + interpolate from start to finish

Handle delay-and-return prediction as two messages generated at the same time, queued on start tic

---

# Anything more?

## A few optimizations

- Move tic-of-validity field to packet header
  - The sender's view of the receiver's *locally effective* tic at the time the messages' data was generated
  - Enough regularly-occurring messages need this information now to make it worthwhile
- Ack message

## Protocol state machine

Refactor connection, disconnection.

## Evaluate key exchange, signatures, and encryption tools/libraries

