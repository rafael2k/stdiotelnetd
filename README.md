# Stdiotelnetd

This is a simple TCP server wrote around `libtelnet` library and using telnet
protocol for the communication. By default it waits on a given TCP port and when
contacted by a telnet client, it enables interactive communication between
the users sitting on either side of the connection. It can also execute a
program specified as a command line argument (with its own arguments passed
through as needed) having both `stdin`/`stdout` of this program redirected to
any telnet client connected. Note that only one instance of the program will
ever be spawned, hence all of the connected clients will be seeing the same
content, effectively being able to observe the effects of the other connected
users actions (namely, if the spawned program responds with echo, all the users
will be seeing what the others are typing).

## Usage guidelines

The command line syntax for invoking `stdiotelnetd` is following:

```
stdiotelnetd <waitport> [<cmd> [-- [<args>]]]
```

By default, after being connected, the telnet client programs are instructed
not to use local echo and give up the line mode. This means, all the keystrokes
are sent immediately (without waiting for the Enter key), and the effect of
those keystrokes will not be seen on the client side unless the server side
start to send them back. Both those behaviours can be prevented by setting the
following environment variables:

- `TELNET_TELOPT_ECHO` - encourage clients to use local echo
- `TELNET_TELOPT_LINEMODE` - encourage clients to work in the line mode (any
text will be sent to the server only after the user presses the Enter key)

For example, the following command:

```
$ TELNET_TELOPT_ECHO=1 TELNET_TELOPT_LINEMODE=1 ./stdiotelnetd 2048
```

...will start `stdiotelnetd` that will wait at TCP port 2048 and will not be
instructing the telnet clients to stop outputing local echo and give up line
mode.

## How to build it?

This program requires `libtelnet` library. Depending on the version you may
came across, it can provide either `pkg-config` or `CMake` guidance files.
For that reason, both the `Makefile` and `CMakeLists.txt` files were prepared.

If your `libtelnet` library is `pkg-config` compatible, just run `make` to build
this program.

If your `libtelnet` library is `CMake` compatible, use `CMake` as such:

```
$ mkdir build
$ cd build
$ cmake /path/to/stdiotelnetd/sources
$ make
```

## Caveats

Does not work well with Putty. Use any usual command-line telnet client instead.
