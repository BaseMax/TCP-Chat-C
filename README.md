# TCP-Chat-C

A lightweight and efficient TCP chat server built with C. This project demonstrates how to handle multiple client connections, broadcast messages, and manage user sessions using C's high-performance networking capabilities.

## Features

- 🚀 Blazing-fast TCP server powered by C
- 👥 Multi-user support with real-time message broadcasting
- 🔒 Automatic user registration with nickname validation
- 🔌 Handles disconnections gracefully
- 🎉 Welcomes new users and provides an online user list

## Installation

Make sure you have C compiler installed on your system.

**Clone the repository:**

```
git clone https://github.com/BaseMax/TCP-Chat-C.git
cd TCP-Chat-C
```

**Compile:**

```
gcc main.c -o main
```

## Usage

To start the server, run:

```
./main
```

By default, the server runs on `127.0.0.1:3000`. You can connect to it using a TCP client like `nc` (Netcat):

```
nc 127.0.0.1 3000
```

## How It Works

When a client connects, they are prompted to enter a nickname.

If the nickname is unique, they join the chatroom and receive a welcome message.

Messages sent by a user are broadcasted to all other connected clients.

If a user disconnects, a notification is sent to the remaining users.

## Project Structure

- `main.c` - Main entry point of the TCP server

## Contributing

Contributions are welcome! Feel free to fork the repo, create a feature branch, and submit a pull request.

## License

This project is open-source and available under the MIT License.

Copyright 2025, Max Base
