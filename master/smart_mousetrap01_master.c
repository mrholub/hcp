// Code adapted from example by Gert van Loo & Dom
// Example code from: http://elinux.org/Rpi_Low-level_peripherals#C_2


#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>


//#include <time.h> //for random values being random

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

int  mem_fd;
void *gpio_map;

// I/O access
volatile unsigned *gpio;


// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

//#define GPIO_READ(g) *(gpio + 13) &= (1<<(g))


// GPIO ports for seral transfer

#define MISO 18
#define MOSI 23
#define SCK 24 
#define ACK 25
#define CS 4

// MACROS for MISO/MOSI/SCK/ACK operations

//output
#define SET_MOSI_HI  GPIO_SET=(1 << MOSI); 
#define SET_MOSI_LO  GPIO_CLR=(1 << MOSI);

#define SET_SCK_HI  GPIO_SET=(1 << SCK);
#define SET_SCK_LO  GPIO_CLR=(1 << SCK);

#define SET_CS_HI  GPIO_SET=(1 << CS);
#define SET_CS_LO  GPIO_CLR=(1 << CS);

//input
#define GPIO_LEV *(gpio+13)                  // pin level

// Command macros

#define ECHO 254


#define DELAY_S_TRANSFER 1000 // delay between operations during the transfer (us)
#define TRANSFER_VERBOSITY 0  // print actions during the transfer

// Global variables

int byte_out=0;
int byte_in=0;

char device_id[255];
char command_list[255];
//
// Set up a memory regions to access GPIO
//
void setup_io()
{
    /* open /dev/mem */
    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
        printf("can't open /dev/mem \n");
        exit(-1);
    }

    /* mmap GPIO */
    gpio_map = mmap(
          NULL,                 //Any adddress in our space will do
          BLOCK_SIZE,           //Map length
          PROT_READ|PROT_WRITE, // Enable reading & writting to mapped memory
          MAP_SHARED,           //Shared with other processes
          mem_fd,               //File to map
          GPIO_BASE             //Offset to GPIO peripheral
    );

    close(mem_fd); //No need to keep mem_fd open after mmap

    if (gpio_map == MAP_FAILED) {
          printf("mmap error %d\n", (int)gpio_map); //errno also set!
          exit(-1);
    }

    // Always use volatile pointer!
    gpio = (volatile unsigned *)gpio_map;
} // setup_io()


void setup_io();

int read_gpio(int pin){ 

unsigned int read_gpio_value = GPIO_LEV;               // reading all 32 pins
int pin_value = ((read_gpio_value & (1 << pin)) != 0); // get pin value
//printf ("[read_gpio] pin %d value: %d\n",pin,pin_value);
//sleep(1);
return pin_value;
}


port_init(){
printf ("[port_init] port_init\n");

    // Set up gpi pointer for direct register access
    setup_io();

    // set GPIO pins as output
    INP_GPIO(MISO); // must use INP_GPIO before we can use OUT_GPIO

    INP_GPIO(MOSI); // must use INP_GPIO before we can use OUT_GPIO
    OUT_GPIO(MOSI);

    INP_GPIO(SCK); // must use INP_GPIO before we can use OUT_GPIO
    OUT_GPIO(SCK);

    INP_GPIO(ACK); // must use INP_GPIO before we can use OUT_GPIO

    INP_GPIO(CS); // must use INP_GPIO before we can use OUT_GPIO
    OUT_GPIO(CS);

printf ("[port_init] port_init exit\n");

}

comm_init(){
    SET_MOSI_HI;
    SET_SCK_HI;
}

comm_stop(){
    SET_MOSI_LO;
    SET_SCK_LO;
}

void send_byte(){
if (TRANSFER_VERBOSITY > 0 ){
    printf ("[send_byte] executing send_byte\n");
}
int bit_number_to_send=7;
if (TRANSFER_VERBOSITY > 1 ){
    printf ("[send_byte] SEND BYTE\n");
    printf ("[send_byte] comm_init\n");
}
//comm_init(); //master comm init
if (TRANSFER_VERBOSITY > 1 ){
    printf ("[send_byte] Waiting for ACK\n");
}
SET_SCK_HI;
while (read_gpio(ACK) == 0){;;} //waiting for slave to put ACK HI
    if (TRANSFER_VERBOSITY > 1 ){
    printf ("[send_byte] CHECK_ACK passed\n");
    printf ("[send_byte] Waiting for MISO\n");
    }

SET_MOSI_HI;
while (read_gpio(MISO) == 0){;;} ///waiting for slave to put MISO HI
if (TRANSFER_VERBOSITY > 1 ){
    printf ("[send_byte] CHECK_MISO passed\n");
}
    while ( bit_number_to_send > 0 ){

        //sending of even bit
        if (TRANSFER_VERBOSITY > 1 ){	
        printf ("[send_byte] sending of odd bit %d\n",bit_number_to_send);
        }
	if ( byte_out&(1<<bit_number_to_send)){
            if (TRANSFER_VERBOSITY > 1 ){
      	    printf ("[send_byte] SET_MOSI_HI\n");
            } 
	SET_MOSI_HI;
	}
	else {
	    SET_MOSI_LO;
                if (TRANSFER_VERBOSITY > 1 ){
                printf ("[send_byte] SET_MOSI_LO\n");
                }
	}
	usleep(DELAY_S_TRANSFER);
	SET_SCK_LO;

        if (TRANSFER_VERBOSITY > 1 ){
	    printf ("[send_byte] waiting till ACK is HI\n");		
        }
	while (read_gpio(ACK)){;;} //waiting till ACK is HI
	bit_number_to_send--;

	//sending of odd bit
        if (TRANSFER_VERBOSITY > 1 ){
	    printf ("[send_byte] sending of even bit %d\n",bit_number_to_send );
        }
	if ( byte_out&(1<<bit_number_to_send)){
            if (TRANSFER_VERBOSITY > 1 ){
	        printf ("[send_byte] SET_MOSI_HI\n");
            }
	    SET_MOSI_HI;
	}
	else {
	    SET_MOSI_LO;
            if (TRANSFER_VERBOSITY > 1 ){
            printf ("[send_byte] SET_MOSI_LO\n");
            }
	}
	usleep(DELAY_S_TRANSFER);
	SET_SCK_HI;
		
        if (TRANSFER_VERBOSITY > 1 ){
	    printf ("[send_byte] waiting till ACK is LO\n");
        }
	while (!read_gpio(ACK)){;;} //waiting till ACK is LO
	bit_number_to_send--;
	}

if (TRANSFER_VERBOSITY > 1 ){
    printf ("[send_byte] SET_SCK_LO\n");
    printf ("[send_byte] waiting till CHECK_MISO is LO\n");
}

SET_MOSI_LO;
while (read_gpio(MISO) == 1){;;} //waiting for slave to put MISO LO
if (TRANSFER_VERBOSITY > 1 ){
    printf ("[send_byte] CHECK_MISO passed\n");
}

SET_SCK_LO;
while (read_gpio(ACK) == 1){;;} //waiting for slave to put ACK LO
if (TRANSFER_VERBOSITY > 1 ){
    printf ("[send_byte] CHECK_ACK passed\n");
}

    if (TRANSFER_VERBOSITY > 0 ){
        printf ("[send_byte] exit\n");
    }
return;
}

void receive_byte(){
byte_in=0; //erasing previous byte to add correctly new bits
if (TRANSFER_VERBOSITY > 0 ){
    printf ("[receive_byte] executing receive_byte\n");
}
int bit_number_to_receive=0;
if (TRANSFER_VERBOSITY > 1 ){
    printf ("[receive_byte] RECEIVE BYTE\n");
    printf ("[receive_byte] comm_init\n");
}
//comm_init(); //master comm init

SET_SCK_HI;
while (read_gpio(ACK) == 0){;;} //waiting for slave to put ACK HI
    if (TRANSFER_VERBOSITY > 1 ){
    printf ("[receive_byte] CHECK_ACK passed\n");
    printf ("[receive_byte] Waiting for MISO\n");
    }
SET_MOSI_HI;
while (read_gpio(MISO) == 0){;;} ///waiting for slave to put MISO HI
if (TRANSFER_VERBOSITY > 1 ){
    printf ("[receive_byte] CHECK_MISO passed\n");
}

while ( bit_number_to_receive < 7 ){

//receiving of bit
   if (TRANSFER_VERBOSITY > 1 ){
    printf ("[receive_byte] SET_SCK_LO\n");
    }
    usleep(DELAY_S_TRANSFER);
    SET_SCK_LO;

    if (TRANSFER_VERBOSITY > 1 ){
        printf ("[receive_byte] waiting till ACK is HI\n");
    }
    while (read_gpio(ACK) == 1){;;} //waiting till ACK is HI
    if (TRANSFER_VERBOSITY > 1 ){
        printf ("[receive_byte] receiving of bit %d\n",bit_number_to_receive);
    }
    if (read_gpio(MISO) == 1){ // If pin status is HI ...
        byte_in |= 1 << bit_number_to_receive; // then logical 1 is set in bit.
        if (TRANSFER_VERBOSITY > 1 ){
	    printf ("[receive_byte] receiving of HI BIT\n");
        }
    }
    bit_number_to_receive++;

    if (TRANSFER_VERBOSITY > 1 ){
        printf ("[receive_byte] byte_in= %d\n",byte_in);
    }

    //receiving of bit
    if (TRANSFER_VERBOSITY > 1 ){
    printf ("[receive_byte] SET_SCK_HI\n");
    }
    usleep(DELAY_S_TRANSFER);
    SET_SCK_HI;
    if (TRANSFER_VERBOSITY > 1 ){
    printf ("[receive_byte] waiting till ACK is LO\n");
    }
    while (read_gpio(ACK) == 0){;;} //waiting till ACK is LO
    if (TRANSFER_VERBOSITY > 1 ){
    printf ("[receive_byte] receiving bit number %d\n",bit_number_to_receive);
    }
    if (read_gpio(MISO) == 1){ // If pin status is HI ...
        byte_in |= 1 << bit_number_to_receive; // then logical 1 is set in bit.
        if (TRANSFER_VERBOSITY > 1 ){
            printf ("[receive_byte] receiving of HI BIT\n");
        }
    }
    bit_number_to_receive++;

    if (TRANSFER_VERBOSITY > 1 ){
        printf ("[receive_byte] byte_in= %d\n",byte_in);
    }

    if (TRANSFER_VERBOSITY > 0 ){
        printf ("[receive_byte] loop  exit\n");
    }
}

if (TRANSFER_VERBOSITY > 1 ){
    printf ("[receive_byte] SET_SCK_LO\n");
    printf ("[receive_byte] waiting till MISO is HI\n");
}

SET_MOSI_LO;
while (read_gpio(MISO) == 1){;;} //waiting for slave to put MISO LO
if (TRANSFER_VERBOSITY > 1 ){
    printf ("[receive_byte] CHECK_MOSI passed\n");
}

SET_SCK_LO;
while (read_gpio(ACK) == 1){;;} //waiting for slave to put ACK LO
if (TRANSFER_VERBOSITY > 1 ){
    printf ("[receive_byte] CHECK_ACK passed\n");
}
    if (TRANSFER_VERBOSITY > 0 ){
        printf ("[receive_byte] exit\n");
    }
return;
}

void echo_loop(){

byte_out=0;
int test_number=0;
int test_value=255;

    SET_CS_LO;
    usleep(1);
    SET_CS_HI;
    printf ("[echo_loop] SET_CS_HI\n");
    while (1){
        printf ("[echo_loop] ===========START\n");
        byte_out=254;//echo
        send_byte();
        // printf ("[main] 1 byte sent\n");
        byte_out=test_value; //testing value to send
        send_byte();
	//printf ("[main] 2 byts sent\n");
        receive_byte(); //receiving answer - should equal to testing value
        printf ("[echo_loop] test_number ==  %d\n",test_number);
        printf ("[echo_loop] byte_out ==  %d\n",byte_out);
        printf ("[echo_loop] byte_in ==  %d\n",byte_in);
        printf ("[echo_loop] ==================END\n");
        if (byte_in != test_value){
        printf("comm error\n");
        break;
    }
        test_number++;
        test_value++;
        if (test_value > 255){
            test_value=0;
        }
        byte_out++;
    }
}

void get_device_id(){
if (TRANSFER_VERBOSITY > 0 ){
printf ("[get_device_id] ===========START\n");
}
int device_id_l; //length of device ID
int sp;

SET_CS_LO;
usleep(1000);
SET_CS_HI;
if (TRANSFER_VERBOSITY > 0 ){
printf ("[get_device_id] SET_CS_HI\n");
}

byte_out=1;
send_byte();
byte_in=0;

//receives all chars of device_id and store these in array.
while (byte_in != ';'){
   receive_byte();
   printf ("%c",byte_in);
}

printf ("\n");

    if (TRANSFER_VERBOSITY > 0 ){
    printf ("[get_device_id] ==================END\n");
    }
SET_CS_LO;
}

void receive_string(){

    if (TRANSFER_VERBOSITY > 0 ){
    printf ("[receive_string] ===========START\n");
    }

SET_CS_LO;
usleep(1000);
SET_CS_HI;
if (TRANSFER_VERBOSITY > 0 ){
printf ("[receive_string] SET_CS_HI\n");
}

//byte_out=10;
send_byte();
byte_in=0;

//receives all chars of device_id and store these in array.
while (byte_in != ';'){
   receive_byte();
   printf ("%c",byte_in);
}

    printf ("\n");

    if (TRANSFER_VERBOSITY > 0 ){
    printf ("[receive_string] ==================END\n");
    }
SET_CS_LO;
}

void get_cmd_list(){

    if (TRANSFER_VERBOSITY > 0 ){
    printf ("[get_cmd_list] ===========START\n");
    }

SET_CS_LO;
usleep(1000);
SET_CS_HI;
if (TRANSFER_VERBOSITY > 0 ){
printf ("[get_cmd_list] SET_CS_HI\n");
}

byte_out=2;
send_byte();
byte_in=0;

//receives all chars of device_id and store these in array.
while (byte_in != ';'){
   receive_byte();
   printf ("%c",byte_in);
}

    printf ("\n");

    if (TRANSFER_VERBOSITY > 0 ){
    printf ("[get_cmd_list] ==================END\n");
    }
SET_CS_LO;
}

int main(int argc, char **argv){
port_init();

int command;

while (1){
printf ("Select: ");
scanf("%d", &command);
     switch(command){
        case 0 :
            printf ("Status\n");
            break;
         case 1 :
            get_device_id();
            break;
         case 2 :
            get_cmd_list();
            break;
         case 10 :
            byte_out=10;
            receive_string();
            break;
         case 11 :
            byte_out=11;
            receive_string();
            break;
          case 12 :
            byte_out=12;
            receive_string();
            break;
       case 254 :
            echo_loop();
            break;
        default :
           printf ("Invalid selection.\n");
    }
}

echo_loop();

}
