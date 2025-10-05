# Multiplayer-Online-Asynchronous-Battleship-MOAB-
A game server for MOAB written in **C** , designed to run on any Linux machine or WSL environment.  
It can also be hosted remotely using **SSH tunneling and port forwarding** for secure client connections.

This is a network-based asynchronous multiplayer Battleship game server built in C using sockets and I/O multiplexing. It allows multiple clients to connect and play concurrently.


# Message Syntax
Since this game is run on the terminal, all actions are performed through text-based commands(strings):
  Registration: 
    player sends: ("REG %20s %d %d %c\n", name, x, y, d).
                  - `name`: up to 20 characters (letters, digits, or '-')  
                  - `(x, y)`: center of the ship (10x10 grid)  
                  - `d`: '-' for horizontal (1x5), '|' for vertical (5x1)

                  **Examples:**
                  - `(3,4), '-' →` occupies `(1,4)-(5,4)`
                  - `(3,4), '|' →` occupies `(3,2)-(3,6)`

**Server Responses:**
          - `TAKEN\n` → name already used  
          - `INVALID\n` → ship placement out of bounds  
          - `WELCOME\n` → successful registration  
          - `JOIN %s\n` → broadcast to all players

### Bombing
**Player → Server:**
    Bombing: Player can request a bomb using ("BOMB %d %d\n", x, y)
    Server: If syntax error, the server replies ("INVALID\n")
            Server damage report: ("HIT %s %d %d %s\n", attacker, x, y, victim)
            Server miss report: ("MISS %s %d %d\n", attacker, x, y)
            once a all of a players ship tiles have been hit or the player disconnects server Braodcasts: ("GG %s\n", player)
# Bad clients: the situation that aren't covered above
    Sending a message to a client causes an error, SIGPIPE, or would block by the client: Disconnect
    Message too long: Even the longest valid client message has a maximum possible length, if the limit is reached the player gets Disconncted.


# How to Run: 
  Compile the files using gcc -O2 *.c.
  then run the out file using ./a.out <port number>
  the server will bind() the port number.
  if bind() gave “address in use” error, try another port number
# Play remotely
  Supports SSH-based port forwarding and tunneling for remote gameplay.
  can run using:
    ssh -R <remote_port>:<local_address>:<local_port> user@remotehost
    i.e. 
    ssh -R 14758:127.0.0.1:8080 user@remotehost

  This project uses standard POSIX socket and multiplexing APIs (no external dependencies). It can be compiled on any Unix-based system or WSL using GCC
  Also, The project supports remote testing via SSH tunneling and port forwarding, enabling connections from external machines to the server’s listening socket.
  
FIle Structure
/src
 ├── server.c       # Main server logic
 ├── player.c       # Player management and validation
/include
 └── player.h
README.md
Makefile

#Libraries and Tools Used
* POSIX sockets
* I/O multiplexing (poll/select)
* nc and SSH for testing

No external dependencies required. The server can be compiled using GCC on any Unix-based or WSL environment.
Future Work:
  A game chat for players to communicate.
            

                  
    


