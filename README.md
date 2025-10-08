## necho

### About

network echo, or _necho_ (super clever ikr), is a cli widget that writes to stdout the icmp echo reply data from a server.

### Usage

```bash
$ necho
#> usage: necho [HOST] [MESSAGE...]

$ necho 8.8.8.8 "if you're reading this, I'm alive."
#> if you're reading this, I'm alive.

$ necho 127.0.0.1 look mom\! no quotes
#> look mom! no quotes

$ necho 9.8.7.6.5.4.3.2.1 lift off
#> sendto: Permission denied
```

### Install

```bash
$ git clone https://github.com/bchadwic/necho.git
$ gcc -o necho main.c
$ sudo mv necho /usr/local/bin/ && sudo setcap cap_net_raw+ep /usr/local/bin/necho
```
