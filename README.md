At this moment my chat can handle multiple communications between server and clients. We can receive a message from a client and send it to the others, I have been working on the check status task and it's already checking every 5 seconds if the client sends a message or not but it's not working with signals yet. I also have been working in the message queues, and it's missing a source code to send a message to the queue.

Instructions for using make command:
$make <command>

commands can be :
For compile:
server or client to compile for the host;
embServer or embClient to compile for the raspberry pi;


For Run:
runclient <address> <port>
runserver <port>

The make file has others functions but is not ready yet to be used.
