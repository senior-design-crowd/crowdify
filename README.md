Use the source, Luke.

The python client program will connect directly to the MONGODB instance on the RPi. The only issue is that Bob specifically recommended making the client program gutless. The most gutless way to do this would be to use SFTP to connect remotely and pull the file that way. It doesn't change the program too much, though. We just need libssh2.

It seems that the disk sector size depends on the disk label. Disks with MSDOS labels generally have a 512 byte sector size. GPT disks a 4KiB sector size. I'm thinking that, in the name of the future, we should ignore the difference and always go with the larger size. GPT is going to take over everything else.

The way to determine sector size on the go is to run "statvfs" on any file in a given file system. Only hangup is the file system must be mounted.

Sources:
https://en.wikipedia.org/wiki/Master_boot_record

The RPi is found using an ARP. It'll have a constant MAC.

It may be that it's better to store a sectors unit of data, whatever that means for the hard drive in question. This allows for greater flexibility with tools down the road. We can simply add a field to each record and a condition to the existance test.

It occurs to me that once this data is available in sector size units, there's nothing stoping us from making a variety of clients that do various things relating to hot topics like digital forensics. We can also make a backup tool that has the characteristics of rsync, bacula, and dd. This creates a nice way for us to test out a bunch of crazy features, without too much effort.

There is absolutely no reason we shouldn't multithread block insertion

#Components
	- mongodb: the database
	- vacuum: transfers blocks into the database
	- broom: cleans blocks that are no longer referenced
	- replicator: duplicates blocks across nodes
	- minion: a web client, providing acesss to the server settings

Once we have this data, we can rebuild operating systems easily. We may be able to build stock copies. Maybe build logical "Stock Volumes"
