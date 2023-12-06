#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>

#include "../include/cyusb.h"

static cyusb_handle *h1 = NULL;
unsigned short int gl_ep_in, gl_ep_out;

void bulk_reader(int num_bytes)
{
    int rStatus, nbr;
    unsigned char *buf;
    int transferred = 0;

    buf = (unsigned char*)(malloc(num_bytes * 2));
    if (buf == NULL){
        printf ("Error in Memory allocation ... Try a smaller size\n");
        return;
    }

    rStatus = cyusb_bulk_transfer(h1, gl_ep_in, buf, num_bytes, &transferred, 2000);
    if (rStatus == LIBUSB_ERROR_TIMEOUT){
        printf ("\nRead only %d bytes \n", transferred);
        printf ("\nError: Timed out while reading %d bytes ..\n\n", (num_bytes - transferred)); 
        cyusb_clear_halt (h1, gl_ep_in);
        free (buf);
        return; 
    }
    else if (rStatus == LIBUSB_ERROR_OVERFLOW){
        printf ("\nBuffer overflow occurred read only %d bytes \n\n", transferred);
        printf ("Clearing ep %d \n",gl_ep_in);
        cyusb_clear_halt (h1, gl_ep_in);
        free (buf);
        return;
    }
    else if (rStatus != LIBUSB_SUCCESS){
        printf ("\nError in doing bulk read ..read %d bytes only %d \n", transferred, rStatus);
        cyusb_clear_halt (h1, gl_ep_in);
        free (buf);
        return;
    }

    printf ("\nSuccessfully read %d bytes \n\n", transferred);
    free (buf);
}

int main(int argc, char **argv)
{
    int rStatus;
    libusb_config_descriptor *configDesc;
    const struct libusb_endpoint_descriptor *endpoint;
    const struct libusb_interface *interface0 ;
    int num_bytes, maxPacketSize, numEndpoints, ep_index_out = 0, ep_index_in = 0;
    unsigned short int temp, ep_in[8], ep_out[8];

    if (argc != 2){
        printf ("Usage is ./cybulk_reader <num of bytes> \n Ex ./cybulk_reader 1024 \n");
        return -1;
    }

    rStatus = cyusb_open();
    if ( rStatus < 0 ) {
        printf("Error opening library\n");
        cyusb_close();
        return -1;
    }
    else if ( rStatus == 0 ) {
        printf("No device found\n");
        cyusb_close();
        return 0;
    }

    num_bytes = atoi (argv[1]);
    printf ("Number of bytes is %d ", num_bytes);
    h1 = cyusb_gethandle(0);
    rStatus = cyusb_kernel_driver_active(h1, 0);
    if ( rStatus != 0 ) {
        printf("kernel driver active. Exitting\n");
        cyusb_close();
        return 0;
    }
    rStatus = cyusb_claim_interface(h1, 0);
    if ( rStatus != 0 ) {
        printf("Error in claiming interface\n");
        cyusb_close();
        return 0;
    }
    rStatus = cyusb_get_config_descriptor (h1, 0, &configDesc);
    if (rStatus != 0){
        printf ("Could not get Config descriptor \n");
        cyusb_close();
        return -1;
    }

    //Finding the endpoint address
    interface0 = configDesc->interface;
    numEndpoints = interface0->altsetting->bNumEndpoints;
    endpoint = interface0->altsetting->endpoint;

    while (numEndpoints){
        if (endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN){
            ep_in [ep_index_in] = endpoint->bEndpointAddress; 
            ep_index_in++;
        }
        else{
            ep_out [ep_index_out] = endpoint->bEndpointAddress;
            ep_index_out++;
        }
        numEndpoints --;
        endpoint = ((endpoint) + 1);
    }
    //Choosing 1 endpoint for read and one for write
    if (ep_in[0] == 0){
        printf ("No IN endpoint in the device ... Cannot do bulk write\n");
        cyusb_free_config_descriptor (configDesc);
        cyusb_close();
        return -1;    
    }

    gl_ep_in = ep_in [0];
    gl_ep_out = ep_out [0];

    printf ("The Endpoint address is  0x%x \n", gl_ep_in);
    maxPacketSize = cyusb_get_max_packet_size (h1, gl_ep_in);
    if (((num_bytes % maxPacketSize) + 1) != 1){
        printf ("Number of bytes to read should be multiple of %d--The EP max packetsize \n", maxPacketSize);
        cyusb_free_config_descriptor(configDesc);
        cyusb_close();
        return -1;
    }
    bulk_reader (num_bytes);
    cyusb_free_config_descriptor(configDesc);
    cyusb_close();
    return 0;
}
