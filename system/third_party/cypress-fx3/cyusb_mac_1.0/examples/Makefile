all:
	g++ -o cyisowrite_test	        cyisowrite_test.c	        -l cyusb -l usb-1.0	
	g++ -o cyisoread_test	        cyisoread_test.c	        -l cyusb -l usb-1.0
	g++ -o cybulk_reader	        cybulk_reader.c             -l cyusb
	g++ -o cybulk_writer	        cybulk_writer.c	            -l cyusb
	g++ -o cybulkread_performance   cybulkread_performance.c    -l cyusb 
	g++ -o cybulkwrite_performance  cybulkwrite_performance.c   -l cyusb 
	g++ -o download_fx2             download_fx2.c              -lcyusb
	g++ -o download_fx3             download_fx3.c              -lcyusb

clean:
	rm -f cyisowrite_test cyisoread_test cybulk_reader cybulk_writer \
	    cybulkread_performance cybulkwrite_performance download_fx3 download_fx2

help:
	@echo	'make		would compile all the programs correctly'
	@echo	'make clean	would remove all compiled programs'
