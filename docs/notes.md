1) In the first stage of ft_irc, the focus is on creating the foundational components of the IRC Server. This includes setting up a communication endpoint and signal handling mechanisms.

What is a socket?

IPC (inter-process communication)

There are various methods that processes can use to communicate with each other, and some projects explore some of these methods. For example, pipes are used in Pipex, signals in Minitalk, semaphores in Philosophers, and of course, sockets in ft_irc and webserv.

A socket is an endpoint that enables two processes to communicate with each other, either on the same machine or across a network. It acts as an interface between the application layer and the transport layer of the networking stack.



# TYPES OF PORTS

The reason for this range is that in TCP and UDP, a port number is represented by a 16-bit unsigned integer, and there are three types of ports.

* Ports 0 to 1023 are reserved for specific services and protocols, such as HTTP (port 80), FTP (port 21), and SSH (port 22). They require administrative privileges to use.


* Ports numbered 1024 to 49151 can be registered for specific purposes and are used by non-standard applications and services.

* Dynamic or private ports (49152 to 65535) are used by client applications for outgoing connections. These ports are dynamically allocated by the operating system to clients when they initiate outgoing connections.