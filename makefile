#Compiler flags:
# -g    adds debugging information to the executable file
# -Wall turns on most, but not all, compiler warnings
CFLAGS = -Wall -g

#Extra flags to give to compilers when they are supposed to invoke the linker, ‘ld’, such as -L.
#Libraries (-lfoo) should be added to the LDLIBS variable instead.
#LDFLAGS = -L.

#Library flags or names given to compilers when they are supposed to invoke the linker, ‘ld’.
#Non-library linker flags, such as -L, should go in the LDFLAGS variable.
LDLIBS = -lpigpio -lmpdclient

#The target executable file name
TARGET = rad10d

#The compiler; typically gcc for c and g++ for c++
CC = gcc

all:
	$(CC) $(CFLAGS) $(TARGET).c $(LDLIBS) -o $(TARGET)

#Executing "make clean" will carry out the following.
clean:
	rm --force $(TARGET) *.o

#Installation destinations.
INSTALL_DEST_BIN = /usr/local/sbin/$(TARGET)
INSTALL_DEST_SERVICE = /lib/systemd/system/$(TARGET).service

#Executing "make install" will carry out the following.
install: all
ifneq ($(shell id -u), 0)
	@echo Must be run as root.  Try: sudo make install
else
	install --mode=0755 --owner=root --group=root $(TARGET) $(INSTALL_DEST_BIN)
	install --mode=0755 --owner=root --group=root $(TARGET).service $(INSTALL_DEST_SERVICE)
	systemctl enable $(TARGET).service
	systemctl start $(TARGET).service
endif

#Executing "make uninstall" will carry out the following.
uninstall:
ifneq ($(shell id -u), 0)
	@echo Must be run as root.  Try: sudo make uninstall
else
	systemctl disable $(TARGET).service
	systemctl stop $(TARGET).service
	rm --force $(INSTALL_DEST_BIN)
	rm --force $(INSTALL_DEST_SERVICE)
endif
