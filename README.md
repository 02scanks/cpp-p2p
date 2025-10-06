# C++ P2P Chat

A lightweight, decentralized peer-to-peer chat application written in C++ using raw TCP sockets. Each peer acts as both a client and server, enabling direct communication without a central server.

## Features

### Current Implementation

- **True P2P Architecture** - Every peer can both accept incoming connections and connect to other peers
- **Multi-Peer Support** - Connect to multiple peers simultaneously with automatic message relaying
- **Username-Based Identity** - Each peer has a unique username for the session
- **Thread-Safe Communication** - Mutex-protected peer list for concurrent access
- **Custom Message Protocol** - Header-based protocol with type identification and length-prefixing
- **Automatic Message Relay** - Messages are automatically forwarded to all connected peers
- **Clean Disconnection Handling** - Peers are properly removed from the network on disconnect

## Architecture

### Threading Model

```
Main Thread
├─ Listening Thread (accepts new peer connections)
│  ├─ Peer Handler Thread 1 (receives messages from peer 1)
│  ├─ Peer Handler Thread 2 (receives messages from peer 2)
│  └─ ...
└─ User Input Loop (reads from stdin and broadcasts)
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

This dual-type system prevents duplicate username prefixing when messages are relayed through intermediate peers.

### Connection Flow

1. **Peer A starts:** Listens on port (e.g., 8080)
2. **Peer B connects:** Connects to Peer A's IP:8080
3. **Username Exchange:**
   - Peer B sends username → Peer A receives
   - Peer A sends username → Peer B receives
4. **Bidirectional Communication:** Both peers can now send/receive messages
5. **Multi-Peer:** If Peer C connects to Peer A, messages are relayed between all peers

## Building

### Prerequisites

- C++11 or later
- POSIX-compliant system (Linux/macOS)
- Standard libraries: `pthread`, `socket`

### Compile

```bash
g++ -std=c++11 -pthread peer.cpp -o p2p
```

## Usage

### Starting a Peer

```bash
./p2p
```

You'll be prompted to:
1. Enter a username
2. Enter a port to listen on
3. Choose whether to connect to an existing peer

### Example: Two-Peer Chat

**Terminal 1 (Alice):**
```
./p2p
Enter a username for this session
alice
Enter a port to listen on
8080
Connect to peer? (y/n):
n
Listening Initialized
Awaiting Connections....
```

**Terminal 2 (Bob):**
```
./p2p
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
```

Now both Alice and Bob can type messages and they'll be relayed to each other.

### Example: Three-Peer Network

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

## Technical Details

### Thread Safety

- Peer list is protected with `std::mutex`
- All read/write operations acquire locks before accessing shared data
- Peers are added to the list before spawning handler threads to avoid race conditions

### Socket Management

- Uses `SO_REUSEADDR` to allow quick rebinding after crashes
- Non-blocking accept loop for handling multiple incoming connections
- Proper socket cleanup on peer disconnection

### Protocol Design

The header-based protocol handles chunked TCP data:
1. Read data until two `|` delimiters are found
2. Parse header: `TYPE|LENGTH|`
3. Extract payload (may span multiple recv() calls)
4. Process based on type (MSG vs RELAY)

## Project Goals

### Completed ✓

- [x] Basic P2P text messaging
- [x] Multi-peer support with message relaying
- [x] Username exchange protocol
- [x] Thread-safe peer management
- [x] Clean disconnection handling
- [x] Message protocol with type discrimination

### In Progress / Planned

- [ ] File transfer support
- [ ] Encryption (TLS/SSL)
- [ ] Peer discovery mechanism
- [ ] NAT traversal
- [ ] Message history
- [ ] GUI interface
- [ ] Group chat rooms
- [ ] Authentication/authorization

## Known Limitations

- No encryption - all messages sent in plaintext
- No peer discovery - must manually enter IP addresses
- No NAT traversal - requires direct network connectivity or port forwarding
- Single-threaded user input - blocking on stdin
- No message persistence
- Fixed 1024-byte buffer size

## File Structure

```
cpp-p2p/
├── peer.hpp       # Peer class declaration and PeerConnection struct
├── peer.cpp       # Implementation and main()
└── README.md      # This file
```

## License

This project is a learning exercise in C++ network programming and P2P architecture.

## Contributing

This is a personal learning project, but feedback and suggestions are welcome!
