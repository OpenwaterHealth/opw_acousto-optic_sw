#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/time.h>

#include "../include/cyusb.h"


int main(int argc, char **argv)
{
    int rStatus;
    cyusb_handle *h1 = NULL;
    libusb_config_descriptor *configDesc;
    const struct libusb_endpoint_descriptor *endpoint;
    const struct libusb_interface *interface0 ;
    unsigned short int temp;
    unsigned short int ep_in, ep_out;
    int transferred = 0, countLimit;
    unsigned char buf [1024*1024*4];
    int nbr = 1024*(1024 * 4),count = 0, read_test = 0;
    long sec,usec;
    struct timeval tv_start, tv_end;

    if (argc != 2){
        printf ("Usage ./cybulkwrite_performance <Count of 4mb records sent to device> \n Ex: ./cybulkwrite_performance 800 \n Host will send 4 * 800 mb to Device \n"); 
        return -1;
    }

    printf ("***IMPORTANT \n To Run Write performance test Make sure FX3/FX2 has been loaded with bulksrcsink firmware \nElse Test will fail \n\n");

    countLimit = atoi (argv[1]);

    if (buf == NULL){
        printf ("Resource allocation Error ... Try lesser number of bytes \n");
        return -1;
    }

    memset (buf, 0xAA, 4*1024*1024); 

    rStatus = cyusb_open();
    if ( rStatus < 0 ) {
        printf("Error opening library\n");
        return -1;
    }
    else if ( rStatus == 0 ) {
        printf("No device found\n");
        cyusb_close();
        return -1;
    }

    h1 = cyusb_gethandle(0);

    rStatus = cyusb_kernel_driver_active(h1, 0);
    if ( rStatus != 0 ) {
        printf("kernel driver active. Exitting\n");
        cyusb_close();
        return -1;
    }
    rStatus = cyusb_claim_interface(h1, 0);
    if ( rStatus != 0 ) {
        printf("Error in claiming interface\n");
        cyusb_close();
        return -1;
    }
    else fprintf(stderr,"Successfully claimed interface\n");
    rStatus = cyusb_get_config_descriptor (h1, 0, &configDesc);
    if (rStatus != 0){
        printf ("Could not get Config descriptor \n");
        cyusb_close ();
        return -1;
    }

    interface0 = configDesc->interface;
    endpoint = interface0->altsetting->endpoint;
    ep_in = interface0->altsetting->endpoint->bEndpointAddress;
    endpoint = ((interface0->altsetting->endpoint) + 1);
    ep_out = endpoint->bEndpointAddress;

    if (ep_out & 0x80){
        temp = ep_in;
        ep_in = ep_out;
        ep_out = temp;
    }
    gettimeofday (&tv_start, NULL);	
    while (count < countLimit){
        rStatus = cyusb_bulk_transfer(h1, ep_out, buf, nbr, &transferred, 1000);
        if (transferred == 0){
            printf ("Error in Write ... could not write anything \n\n");
            break;
        }
        if ((rStatus != 0)|| (transferred != nbr))
            printf ("Error in Write..wrote only %d bytes out of %d bytes...\n", transferred, nbr);
        count++;
    }
    if ( rStatus == 0 ) {
        gettimeofday (&tv_end, NULL);
        sec = tv_end.tv_sec - tv_start.tv_sec;        
        usec = tv_end.tv_usec - tv_start.tv_usec;
        	
        if (tv_end.tv_usec < tv_start.tv_usec){
            sec--;
            usec = (1000000 + usec);
        }
        printf ("Transfered Successfully (%d * %d) bytes in  %ld.%ld seconds \n",nbr,count,sec,usec);
    }
    else {
        cyusb_free_config_descriptor (configDesc);
        cyusb_error (rStatus);
        cyusb_close ();
        return rStatus;
    }
    cyusb_free_config_descriptor (configDesc);
    cyusb_close();
    return 0;
}
