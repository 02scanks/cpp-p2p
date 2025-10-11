# NullChat - C++ P2P Chat

A lightweight, decentralized peer-to-peer chat application written in C++ using raw TCP sockets with a modern terminal GUI. Each peer acts as both a client and server, enabling direct communication without a central server.

<p align="center">
  <img src="https://github.com/user-attachments/assets/a3e0affd-dbd2-4677-9c46-585eae1bb6b1" width="470" height="515" alt="screenshot" />
</p>


## Features

### Current Implementation

- **Modern Terminal GUI** - Full-screen FTXUI-based interface with real-time chat and peer display
- **Gossip-Based Peer Discovery** - Automatically discover peers through network gossip protocol
- **True P2P Architecture** - Every peer can both accept incoming connections and connect to other peers
- **Multi-Peer Support** - Connect to multiple peers simultaneously with automatic message relaying
- **Username-Based Identity** - Each peer has a unique username for the session
- **Thread-Safe Communication** - Mutex-protected peer list for concurrent access
- **Custom Message Protocol** - Header-based protocol with type identification and length-prefixing (MSG, RELAY, PEERLIST)
- **Automatic Message Relay** - Messages are automatically forwarded to all connected peers
- **Clean Disconnection Handling** - Peers are properly removed from the network on disconnect
- **Dual Peer View** - GUI displays both directly connected peers and discovered peers separately

## Architecture

### Threading Model

```
Main Thread (GUI event loop)
├─ Listening Thread (accepts new peer connections)
│  ├─ Peer Handler Thread 1 (receives messages from peer 1)
│  ├─ Peer Handler Thread 2 (receives messages from peer 2)
│  └─ ...
└─ GUI renders on callbacks from peer threads
```

### Message Protocol

**Direct Messages (from user input):**
```
MSG|<payload_length>|<payload>
Example: MSG|5|hello
```

**Relayed Messages (forwarded between peers):**
```
RELAY|<payload_length>|<username>: <payload>
Example: RELAY|11|john: hello
```

**Peer Discovery (gossip protocol):**
```
PEERLIST|<payload_length>|<username>,<ip>,<port>,<discoverer>;...
Example: PEERLIST|32|alice,192.168.1.5,8080,bob;
```

This protocol enables automatic peer discovery through network gossip, with duplicate detection and self-exclusion.

### Connection Flow

1. **Peer A starts:** Listens on port (e.g., 8080)
2. **Peer B connects:** Connects to Peer A's IP:8080
3. **Username Exchange:**
   - Peer B sends username → Peer A receives
   - Peer A sends username → Peer B receives
4. **Peer List Exchange:**
   - Peer A sends list of known peers to Peer B
   - Peer B adds discovered peers to its discovery list
5. **Bidirectional Communication:** Both peers can now send/receive messages
6. **Multi-Peer:** If Peer C connects to Peer A, messages are relayed between all peers
7. **Gossip Discovery:** Peer B can see Peer C in its discovered peers list and connect if desired

## Building

### Prerequisites

- C++17 or later
- CMake 3.14 or later
- POSIX-compliant system (Linux/macOS)
- Standard libraries: `pthread`, `socket`
- FTXUI library (automatically fetched via CMake)

### Compile

```bash
mkdir build
cd build
cmake ..
make
```

This will create the `gui` executable in the build directory.

## Usage

### Starting a Peer

```bash
./build/gui
```

You'll be prompted to:
1. Enter a username
2. Enter a port to listen on
3. Choose whether to connect to an existing peer

Once connected, the terminal GUI will launch showing:
- **Left Panel**: Chat window with message history and input box
- **Right Panel**: Connected peers (green) and discovered peers (yellow)

### Example: Two-Peer Chat

**Terminal 1 (Alice):**
```
./build/gui
Enter a username for this session
alice
Enter a port to listen on
8080
Connect to peer? (y/n):
n
Listening Initialized
Awaiting Connections....
[GUI launches]
```

**Terminal 2 (Bob):**
```
./build/gui
Enter a username for this session
bob
Enter a port to listen on
8081
Connect to peer? (y/n):
y
Enter peer IP: 127.0.0.1
Enter peer port: 8080
Connection Succesful
Listening Initialized
Awaiting Connections....
[GUI launches]
```

Both Alice and Bob can now type messages in their GUI input boxes and press Enter to send.

### Example: Three-Peer Network with Discovery

**Terminal 1 (Alice - Hub):**
```
Port: 8080
Connect: n
```

**Terminal 2 (Bob):**
```
Port: 8081
Connect: y → 127.0.0.1:8080
```

**Terminal 3 (Charlie):**
```
Port: 8082
Connect: y → 127.0.0.1:8080
```

All three peers can now communicate. Messages sent by Bob are relayed through Alice to Charlie and vice versa.

**Peer Discovery in Action:**
- Bob's GUI will show Alice as "Connected" and Charlie as "Discovered"
- Charlie's GUI will show Alice as "Connected" and Bob as "Discovered"
- Bob and Charlie can manually connect to each other if desired, creating a mesh network

## Technical Details

### GUI Architecture

- Built with FTXUI library for modern terminal UI
- Callback-based event system for real-time updates
- Message callbacks trigger GUI refresh via `screen.PostEvent()`
- Separate rendering for chat window and peer list panels
- Input handling with Enter key detection for message sending

### Thread Safety

- Peer list is protected with `std::mutex`
- All read/write operations acquire locks before accessing shared data
- Peers are added to the list before spawning handler threads to avoid race conditions
- GUI callbacks are thread-safe with event posting mechanism

### Socket Management

- Uses `SO_REUSEADDR` to allow quick rebinding after crashes
- Non-blocking accept loop for handling multiple incoming connections
- Proper socket cleanup on peer disconnection

### Protocol Design

The header-based protocol handles chunked TCP data:
1. Read data until two `|` delimiters are found
2. Parse header: `TYPE|LENGTH|`
3. Extract payload (may span multiple recv() calls)
4. Process based on type:
   - **MSG**: Direct message from user input, relay to other peers
   - **RELAY**: Already-relayed message, display only
   - **PEERLIST**: Gossip peer discovery data, parse and add to discovered peers

### Peer Discovery System

- **Gossip Protocol**: When peers connect, they exchange lists of known peers
- **Automatic Deduplication**: Checks for duplicate peers before adding to discovered list
- **Self-Exclusion**: Filters out own peer information from discovery
- **Source Tracking**: Each discovered peer records who shared the information

## Project Goals

### Completed ✓

- [x] Basic P2P text messaging
- [x] Multi-peer support with message relaying
- [x] Username exchange protocol
- [x] Thread-safe peer management
- [x] Clean disconnection handling
- [x] Message protocol with type discrimination (MSG, RELAY, PEERLIST)
- [x] Gossip-based peer discovery mechanism
- [x] Terminal GUI interface (FTXUI)
- [x] Real-time peer list visualization
- [x] Callback-based event system

### Planned

- [ ] Direct connection to discovered peers from GUI
- [ ] File transfer support
- [ ] Encryption (TLS/SSL)
- [ ] NAT traversal
- [ ] Message history persistence
- [ ] Group chat rooms
- [ ] Authentication/authorization
- [ ] Peer list rebroadcasting on new peer join

## Known Limitations

- No encryption - all messages sent in plaintext
- Manual connection required - discovered peers shown but must be connected manually
- No NAT traversal - requires direct network connectivity or port forwarding
- No message persistence or history
- Fixed 1024-byte buffer size
- Peer list updates only on initial connection (not rebroadcasted on network changes)

## File Structure

```
cpp-p2p/
├── peer.hpp                # Peer class declaration, ConnectedPeer and DiscoveredPeer structs
├── peer.cpp                # Peer implementation with networking and protocol logic
├── gui.cpp                 # FTXUI-based terminal GUI application (main entry point)
├── CMakeLists.txt          # CMake build configuration
├── README.md               # This file
├── todo.txt                # Development task list
└── build/                  # Build directory (created by CMake)
    └── gui                 # Compiled executable
```

## License

This project is a learning exercise in C++ network programming and P2P architecture.

## Contributing

This is a personal learning project, but feedback and suggestions are welcome!
