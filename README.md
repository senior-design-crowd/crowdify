Use the source, Luke.

The python client program will connect directly to the MONGODB instance on the RPi. The only issue is that Bob specifically recommended making the client program gutless. The most gutless way to do this would be to use SFTP to connect remotely and pull the file that way. It doesn't change the program too much, though. We just need libssh2.

It seems that the disk sector size depends on the disk label. Disks with MSDOS labels generally have a 512 byte sector size. GPT disks a 4KiB sector size. I'm thinking that, in the name of the future, we should ignore the difference and always go with the larger size. GPT is going to take over everything else.

The way to determine sector size on the go is to run "statvfs" on any file in a given file system. Only hangup is the file system must be mounted.

Sources:
https://en.wikipedia.org/wiki/Master_boot_record

The RPi is found using an ARP. It'll have a constant MAC.
