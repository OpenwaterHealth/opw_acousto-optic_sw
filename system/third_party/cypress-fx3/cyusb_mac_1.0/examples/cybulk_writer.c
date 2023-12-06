#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>

#include "../include/cyusb.h"

static cyusb_handle *h1 = NULL;
unsigned short int gl_ep_in, gl_ep_out;

void bulk_writer(int num_bytes)
{
    int rStatus, nbr;
    unsigned char *buf;
    int transferred = 0;
    
    buf = (unsigned char*)(malloc(num_bytes));
    if (buf == NULL){
        printf ("Error in Memory allocation ... Try small size \n");
        return;
    }

    memset(buf, 0xAA, num_bytes);
    rStatus = cyusb_bulk_transfer(h1, gl_ep_out, buf, num_bytes, &transferred, 1000);
    if (rStatus == LIBUSB_ERROR_TIMEOUT){
        printf ("\nTransferred %d bytes \n", transferred);
        printf ("\nError Time out in transfering %d bytes ...\n\n", (num_bytes - transferred)); 
        cyusb_clear_halt (h1, gl_ep_out);
        free (buf);
        return; 
    }
    else if (rStatus == LIBUSB_ERROR_OVERFLOW){
        printf ("Buffer overflow occurred transferred only %d bytes \n", transferred);
        cyusb_clear_halt (h1, gl_ep_out);
        free (buf);
        return;
    }
    else if (rStatus != LIBUSB_SUCCESS){
        printf ("\nError in doing bulk transfer transfered %d bytes only \n", transferred);
        cyusb_clear_halt (h1, gl_ep_out);
        free (buf);
        return;
    }
    
    printf ("\nSuccessfully transferred %d bytes \n\n", transferred);
    free (buf);
}


int main(int argc, char **argv)
{
    int rStatus;
    libusb_config_descriptor *configDesc;
    const struct libusb_endpoint_descriptor *endpoint;
    const struct libusb_interface *interface0 ;
    int num_bytes, numEndpoints, ep_index_out = 0, ep_index_in = 0;
    unsigned short int temp, ep_in[8], ep_out[8];
    
    if (argc != 2){
        printf ("Usage is ./cybulk_writer <num of bytes> \n Ex ./cybulk_writer 1024 \n");
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
    if (ep_out[0] == 0){
        printf ("No Out endpoint in the device ... Cannot do bulk write\n");
        cyusb_free_config_descriptor (configDesc);
        cyusb_close();
        return -1;    
    }

    gl_ep_in = ep_in [0];
    gl_ep_out = ep_out [0];
    printf ("The Endpoint address is 0x%x \n", gl_ep_out);
    //Calling bulk write function
    bulk_writer (num_bytes);
    //Free up all the descriptor variable
    cyusb_free_config_descriptor (configDesc);
    cyusb_close();
    return 0;
}
