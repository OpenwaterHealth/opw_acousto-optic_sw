all:
	g++ -dynamiclib libcyusb.c -o libcyusb.0.1.dylib -l usb-1.0
	ln -sf libcyusb.0.1.dylib libcyusb.dylib
clean:
	rm -f libcyusb.dylib libcyusb.0.1.dylib
help:
	@echo	'make		would compile and create the library and create a link'
	@echo	'make clean	would remove the library and the soft link to the library (soname)'
