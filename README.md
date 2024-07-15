# Message queue chat app

A C program that runs locally in multiple terminal windows that run it, enabling them to chat with each other.
It demonstrates functionalities from the [*mqueue.h*](https://www.freertos.org/Documentation/api-ref/POSIX/mqueue_8h.html)
header file from the [*C POSIX library*](https://en.wikipedia.org/wiki/C_POSIX_library).

## Installing the code

Copy the repository to a location of your choice.

```shell
cd /some/empty/directory/
git clone https://github.com/vuk1011/message-queue-chat-app.git .
```

## Compiling

```shell
cd /project/location/
gcc main.c utils.c -o main
```

## Executing

You should first be located in the project directory and have the compiled code ready.
When running, the *username* must be passed, no longer than 16 characters, as defined by *USERNAME_LEN*.

```shell
./main chosen_username
```

To be able to chat with each other, all processes must start the program no longer than *TIMEOUT* seconds after the first
one starts it. Otherwise, the late-to-join process will be able to send, but not receive messages. 
The maximum number of participants is defined as a macro as well.

To exit the chat, simply type *exit* as the message.
