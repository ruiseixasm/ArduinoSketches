# JsonTalkie - Walkie-Talkie based Communication for Arduino

A lightweight library for Arduino communication and control using JSON messages over network sockets, with Python companion scripts for host computer interaction.

## Features

- Bi-directional Walkie-Talkie based communication between Arduino and Python
- Simple command/response pattern with "Walkie-talkie" style interaction
- Talker configuration with a Manifesto for self-describing capabilities
- Automatic command discovery and documentation
- Support for multiple devices on the same network


## Installation

### Arduino Library
1. **Using Arduino Library Manager**:
   - Open Arduino IDE
   - Go to `Sketch > Include Library > Manage Libraries`
   - Search for "JsonTalkie"
   - Click "Install"

2. **Manual Installation**:
   - Download the latest release from GitHub
   - Extract to your Arduino libraries folder
   - Restart Arduino IDE


## Python Command Line
### JsonTalkiePy repository with command line as Talker
   - Talker in [JsonTalkiePy](https://github.com/ruiseixasm/JsonTalkiePy)
   - Got the the page above for more details concerning its usage


### Typical usage
```bash
>>> talk
        [talk spy]                 I'm a Spy and I spy the talkers' pings
        [talk test]                I test the JsonMessage class
        [talk blue]                I have no Socket, but I turn led Blue on and off
        [talk nano]                Arduino Nano
        [talk mega]                I'm a Mega talker
        [talk uno]                 Arduino Uno
        [talk green]               I'm a green talker
        [talk buzzer]              I'm a buzzer that buzzes
>>> list nano
        [call nano 0|buzz]         Buzz for a while
        [call nano 1|ms]           Gets and sets the buzzing duration
>>> call nano 0
        [call nano 0]              roger
>>> ping nano
        [ping nano]                3
>>> system nano board
        [system nano board]        Arduino Uno/Nano (ATmega328P)
>>> system nano socket
        [system nano socket]       0       BroadcastSocket_EtherCard
>>> system nano manifesto
        [system nano manifesto]    BlackManifesto
>>> list test
        [call test 0|all]          Tests all methods
        [call test 1|deserialize]          Test deserialize (fill up)
        [call test 2|compare]      Test if it's the same
        [call test 3|has]          Test if it finds the given char
        [call test 4|has_not]      Test if DOESN't find the given char
        [call test 5|length]       Test it has the right length
        [call test 6|type]         Test the type of value
        [call test 7|validate]     Validate message fields
        [call test 8|identity]     Extract the message identity
        [call test 9|value]        Checks if it has a value 0
        [call test 10|message]     Gets the message number
        [call test 11|from]        Gets the from name string
        [call test 12|remove]      Removes a given field
        [call test 13|set]         Sets a given field
        [call test 14|edge]        Tests edge cases
        [call test 15|copy]        Tests the copy constructor
        [call test 16|string]      Checks if it has a value 0 as string
>>> call test 0
        [call test 0]              roger
>>> exit
	Exiting...
```

## JsonTalkie architecture
## Description
The center class is the `MessageRepeater` class, this class routes the JsonMessage between Uplinked
Talkers and Sockets and Downlinked Talkers and Sockets.
## Repeater diagram
The Repeater works in similar fashion as an HAM radio repeater on the top of a mountain, with a clear distinction of Uplinked and Downlinked communications, where the Uplinked nodes are considered remote nodes and the downlinked nodes are considered local nodes.
```
+-------------------------+      +-------------------------+
| Uplinked Sockets (node) |      | Uplinked Talkers (node) |
+-------------------------+      +-------------------------+
                        |          |
                    +------------------+
                    | Message Repeater |
                    +------------------+
                        |          |
+---------------------------+  +---------------------------+
| Downlinked Sockets (node) |  | Downlinked Talkers (node) |
+---------------------------+  +---------------------------+
```

## Talker diagram
```
+--------+
| Talker |
+--------+
       |
     +-----------+
     | Manifesto |
     +-----------+
```

## Message protocol
The extensive list of all Values is in the structure `TalkieCodes`.
### Message Value
These are the Message Values (commands):
- **talk** - Lists existent talkers in the network
- **channel** - Channel management/configuration
- **ping** - Network presence check and latency
- **call** - Action Talker invocation
- **list** - Lists Talker actions
- **system** - System control/status messages
- **echo** - Messages Echo returns
- **error** - Error notification
- **noise** - Invalid or malformed data

### Broadcast Value
Local messages aren't send to uplinked Sockets, except if they are up bridged.
- **none** - No broadcast, the message is dropped
- **remote** - Broadcast to remote talkers
- **local** - Broadcast within local network talkers
- **self** - Broadcast to self only (loopback)

### System Value
This messages are exclusive to the system.
- **undefined** - Unspecified system request
- **board** - Board/system information request
- **mute** - Returns or sets the mute mode
- **drops** - Packet loss statistics
- **delay** - Network delay configuration
- **socket** - List Socket class names
- **manifesto** - Show the Manifesto class name

The `mute` concerns exclusively the `call` commands in order to reduce network overhead

### Repeater Rules
The `MessageRepeater` routes the messages accordingly to its source and message value, the source
comes from the call method `_transmitToRepeater` depending if it's a Socket or a Talker, both here called
nodes.
1. All `remote` messages from `up_linked` nodes are routed to `down_linked` nodes (just);
1. All `remote` messages from `down_linked` nodes are routed to the `up_linked` nodes (just);
1. All `remote` and `local` messages from `up_bridged` nodes are routed to `down_linked` nodes (both);
1. All `remote` and `local` messages from `down_linked` nodes are routed to the `up_bridged` nodes (both);
1. All `local` messages from `down_linked` nodes are also routed to other `down_linked` nodes;
1. All `self` messages from a Talker are routed to the same Talker;
1. All `none` messages are dropped and thus NOT sent to any node.

The configuration of an `up_linked` node into an `up_bridged` one is needed for the case where the
communications are done in the same platform passible of being considered local and thus it shall process
`local` messages too.



