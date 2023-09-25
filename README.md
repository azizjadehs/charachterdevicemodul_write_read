# charachterdevicemodul_write_read
This Repos contains a code which was taken from LDD3 and modified to cover some simple Kernel tasks such as making a charachter device and write and read in it.
The main goal for this Repos is to make use of the example from LDD3 in chapter 3 in a simpler way than it was written in the book.

First you will have to know your kernel version to be able to build this modul and excute the scripts and the Makefile.
To do so use the command $uname -r or follow your kernel directory in your linux system and copy it.

after you know your kernel version then simply modifiy the Makefile: KDIR = /lib/modules/<"your kernel dir>"/build.
other methode to do so are available using linux commands, however I prefered to do it manually.

After that excute the command $make in your terminal. (You will have to first download the package that contain the command make.You can find the necessary packages in https://phoenixnap.com/kb/build-linux-kernel)
for compiling and building.

After successfuly compiling and building, excute the command $insmod .ko to inster our Module.
You can check if the Modul is inserted either by looking at the Syslog or with the command $lsmod.

After inserting the Module, now we need to create our character devices in the /dev directory and then we can use them for reading and writing.
To create these character devices, the provided script dynmodul_load1.sh would is to be excuted. (Make sure you make the script excuteable first by $chmod +x)

Now after creating our character devices we can open another terminal with python and use python to simply test our Module.
For example in python terminal: f = open("name of the character device","w") and then enter.
This will open your character device and a message should appear in the syslog. Then you can use f.write(" ") to write whatever text and save it in the character device.
After that you can reopen the character device for reading this time and with f.read() the text saved in the device would be showen in the userspace in python.

The Module shoud have also print the saved text in the Kernel log with the help of printk, however there was a problem while trying to programm that.

After finishing up with the module you can excute the unload.sh script to clean all created data in the kerenl.
