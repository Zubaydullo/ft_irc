# ft_irc - Internet Relay Chat Server

## Description
ft_irc is a custom implementation of an Internet Relay Chat (IRC) server written in C++. This project implements the core functionality of an IRC server, allowing multiple clients to connect, communicate in channels, and exchange private messages.

## Features
- Multi-client support with non-blocking I/O
- Channel management (create, join, leave, list)
- User authentication and registration
- Private messaging between users
- Channel messaging
- User modes and channel modes
- Basic IRC commands implementation
- Server password protection
- Error handling and logging

## IRC Commands Implemented
### Basic Commands
- `NICK` - Change nickname
- `USER` - Set username and realname
- `PASS` - Server password authentication
- `PING` - Server ping
- `QUIT` - Disconnect from server

### Channel Commands
- `JOIN` - Join a channel
- `PART` - Leave a channel
- `TOPIC` - Set or view channel topic
- `NAMES` - List users in channel
- `LIST` - List all channels
- `INVITE` - Invite user to channel
- `KICK` - Remove user from channel

### Messaging Commands
- `PRIVMSG` - Send private message
- `NOTICE` - Send notice message

### Operator Commands
- `MODE` - Change user or channel modes
- `OPER` - Grant operator privileges

### Bonus Features
- `WHO` - List users matching criteria
- `WHOIS` - Get user information
- `AWAY` - Set away status
- File transfer support (DCC)

## Requirements
- C++98 compatible compiler
- Make
- POSIX-compliant operating system

## Installation
1. Clone the repository:
```bash
git clone https://github.com/yourusername/ft_irc.git
cd ft_irc
```

2. Compile the project:
```bash
make
```

## Usage
Start the server with:
```bash
./ircserv <port> <password>
```

Example:
```bash
./ircserv 6667 mysecretpassword
```

Connect to the server using any IRC client:
```bash
irc://localhost:6667
```

## Project Structure
```
ft_irc/
├── include/          # Header files
├── src/             # Source files
│   ├── server/      # Server implementation
│   ├── client/      # Client handling
│   ├── channel/     # Channel management
│   ├── commands/    # IRC commands
│   ├── utils/       # Utility functions
│   └── message/     # Message handling
├── docs/            # Documentation
└── Makefile         # Build configuration
```

## Testing
The server can be tested using any standard IRC client or using the provided test suite:
```bash
make test
```

## Error Handling
The server implements comprehensive error handling:
- Invalid commands
- Connection errors
- Authentication failures
- Channel operation errors
- User mode errors

## Logging
Server activities are logged to:
- Connection events
- Command execution
- Error messages
- Channel operations

## Contributing
1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Authors
- Your Name - Initial work

## License
This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments
- RFC 1459 - Internet Relay Chat Protocol
- RFC 2810 - Internet Relay Chat: Architecture
- RFC 2811 - Internet Relay Chat: Channel Management
- RFC 2812 - Internet Relay Chat: Client Protocol
- RFC 2813 - Internet Relay Chat: Server Protocol