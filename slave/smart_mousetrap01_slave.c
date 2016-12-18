// Smart mouste trap ver. 1.0
// Fork of Holub's Cummunication Protocol ver. 1.7

/*
M = Master
S = Slave


==== [START] ====


M sends command with pre-defined length.

S responses with packet of pre-defined length.

==== [Command List valid for all devices] ====

= [000] Status report =

M sends [000]
S responds with 1 BYTE:

[000] = OK
[001] = ERROR (unspecified)
[002] = Previous command was not known.
[255] = reserved for Extended status - next byte defines amount of bytes to be sent for whole message.

= [001] Device ID =

M sends [001]

S sends string terminated by ";"

= [002] Device's command list  =

M sends [002]

S sends string terminated by ";"

Each line starts with command number in format [XXX]

= [254] echo for testing purposes =

M sends [254]
M sends another byte
S sends byte back

[255] Extended commands0

reserved for necessity of sending commands with more than 1 BYTE length.
*/

#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>

//#define DELAY_MS_TRANSFER 1
//#define DELAY_MS_END_TRANSFER 0

/* ----------------------- Macros for all ports ------------------------ */
#define SETBIT(ADDRESS,BIT) (ADDRESS |= (1<<BIT))
#define CLEARBIT(ADDRESS,BIT) (ADDRESS &= ~(1<<BIT))
#define CHECKBIT(ADDRESS,BIT) (ADDRESS & (1<<BIT))

/* ----------------------- hardware I/O abstraction ------------------------ */

//Port B
#define MISO 0
#define MOSI 1
#define SCK	2
#define ACK 3
#define CS 4 //Chip select - when 1 communication starts. If gets 0 during communication then communication aborts during waiting state.

#define SET_MISO_HI SETBIT(PORTB,MISO)
#define SET_MISO_LO CLEARBIT(PORTB,MISO)

#define SET_ACK_HI SETBIT(PORTB,ACK)
#define SET_ACK_LO CLEARBIT(PORTB,ACK)

#define CHECK_MOSI_HI CHECKBIT(PINB,MOSI)
#define CHECK_MOSI_LO !CHECKBIT(PINB,MOSI)

#define CHECK_SCK_HI CHECKBIT(PINB,SCK)
#define CHECK_SCK_LO !CHECKBIT(PINB,SCK)

#define CHECK_CS_HI CHECKBIT(PINB,CS)
#define CHECK_CS_LO !CHECKBIT(PINB,CS)



//Port C
#define CLAMP 0
#define SET_CLAMP_HI SETBIT(PORTC,CLAMP)

#define CHECK_CLAMP_HI CHECKBIT(PINC,CLAMP)
#define CHECK_CLAMP_LO !CHECKBIT(PINC,CLAMP)


//Port D
#define LED_BEACON 0
#define SET_LED_BEACON_HI SETBIT(PORTD,LED_BEACON)
#define SET_LED_BEACON_LO CLEARBIT(PORTD,LED_BEACON)

/* ----------------------- global variables ------------------------ */

//communication
int byte_in = 0;
int byte_out = 0;
int  comm_status = 0; // 0 = OK , 1=ERROR abort recent communication.

//device ID - string of device ID and its length (amount of bytes)
char* device_id = "SMART_MOUSE_TRAP_V1;";
// Command list:  all available commands of this device
char* command_list = "[000]STATUS_REPORT\n[001]DEVICE_ID]\n[002]COMMAND_LIST\n[010]TESTING_STRING\n[011]TRAP_STATUS\n[012]TOGGLE_BEACON\n[254]ECHO;";
//char* command_list = "[000];";//
char* teststring = "Mouste trap v.1 IOT super device!;";
// TRAP STAUS - ready or clamp
char* status_ready = "READY;";
char* status_clamp = "CLAMP!;";
// BEACON STATUS
char* beacon_on = "BEACON_ON;";
char* beacon_off = "BEACON_OFF;";
int beacon_status=0; // 1= on ; 2=off
/* ----------------------- hardware initialization ------------------------ */

void port_init(void) { // set up of pins for input/output

    //communication
	DDRB|=(1<<MISO);//set PORTB pin output - MISO
	DDRB&=~(1<<MOSI);//set PORTB pin input - MOSI
	DDRB&=~(1<<SCK);//set PORTB pin input - SCK
	DDRB|=(1<<ACK);//set PORTB pin1 to one as output - ACK
	DDRB&=~(1<<CS);//set PORTB pin input - CS
	//DDRC|=(1<<LED_C0);//set PORTC pin output - LED_C0
    
	//sensor
    DDRC&=~(1<<CLAMP);//set PORTC pin input - CLAMP
	SET_CLAMP_HI; //pull-up resistor up.x

    //leds
	DDRD=0xFF;	//leds
	PORTD=0x00;
	_delay_ms(100); //delay to stabilize everything
}

void comm_init(void){
	SET_MISO_HI;//MISO is set to HI as it's init state of the communication
	SET_ACK_HI;//ACK is set to HI as it's init state of the communication
}

void comm_stop(void){
	SET_MISO_LO;//MISO is set to LO as it's end state of the communication
	SET_ACK_LO;//ACK is set to HI as it's end state of the communication
}

int receive_byte(){ //receive of byte after CLK is triggered to 0 by Master
	
	int i=0;
	byte_in=0;
	//PORTD=0b10000000;

	while (CHECK_SCK_LO){ //waiting for master to set SCK UP before setting ACK and MISO HI.
		if (CHECK_CS_LO){ //IF CS gets LO then abort and set
			byte_in = -1;
			comm_stop(); //MISO and ACK goes LOW
			return byte_in;
		}
	}
	SET_ACK_HI;
	
	while (CHECK_MOSI_LO){ //waiting for master to set SCK UP before setting ACK and MISO HI.
		if (CHECK_CS_LO){ //IF CS gets LO then abort and set
			byte_in = -1;
			comm_stop(); //MISO and ACK goes LOW
			return byte_in;
		}
	}
	SET_MISO_HI;
	
	
	//comm_init();	//setting ACK and MISO high to let master know that slave is ready.
	//PORTD=0b11000000;

	while (i < 4){
		//odd bit
		byte_in = (byte_in<<1); //byte is rotated here in order to not being rotated during last bit.
		while (CHECK_SCK_HI){ //waiting for SCK to be in LOW state
			if (CHECK_CS_LO){ //IF CS gets LO then abort and set
				byte_in = -1;
				comm_stop(); //MISO and ACK goes LOW
				return byte_in;
			}
		}


		if (CHECK_MOSI_HI){ // if logical 1 is received then byte is incremented before rotation
			byte_in++;
		}
		SET_ACK_LO;
		//PORTD=byte_in;
		

		//even bit
		byte_in = (byte_in<<1); //byte is rotated here in order to not being rotated during last bit.
		while (CHECK_SCK_LO){ //waiting for SCK to be in HIGH state
			if (CHECK_CS_LO){
				byte_in = -1;
				comm_stop();//MISO and ACK goes LOW
				return byte_in;
			}
		}
		
		if (CHECK_MOSI_HI){
			byte_in++;
		}
		SET_ACK_HI;
		//PORTD=byte_in;
		
		
		i++;
	}
	//PORTD=0b11100000;
	

	while (CHECK_MOSI_HI){ // !!! waiting for MOSI to be in LOW state after last bit
		//to not run away from function while master is not ready
		if (CHECK_CS_LO){ //IF CS gets LO then abort and set
			comm_stop(); //MISO and ACK goes LOW
			return -1;
		}
	}
	SET_MISO_LO;
	//comm_stop();	//MISO and ACK goes LOW
	
	while (CHECK_SCK_HI){ // !!! waiting for SCK to be in LOW state after last bit
		//to not run away from function while master is not ready
		if (CHECK_CS_LO){ //IF CS gets LO then abort and set
			comm_stop(); //MISO and ACK goes LOW
			return -1;
		}
	}

	SET_ACK_LO;

	//PORTD=0b11111000;
	return byte_in;
}

int send_byte(){ //sending of byte after CLK is triggered to 0 by Master
	
	while (CHECK_SCK_LO){ //waiting for master to set SCK UP before setting ACK and MISO HI.
		if (CHECK_CS_LO){ //IF CS gets LO then abort and set
			byte_in = -1;
			comm_stop(); //MISO and ACK goes LOW
			return byte_in;
		}
	}
	SET_ACK_HI;
	
	while (CHECK_MOSI_LO){ //waiting for master to set SCK UP before setting ACK and MISO HI.
		if (CHECK_CS_LO){ //IF CS gets LO then abort and set
			byte_in = -1;
			comm_stop(); //MISO and ACK goes LOW
			return byte_in;
		}
	}
	SET_MISO_HI;
	
	int bit_number_to_send=0;
	//PORTD=0b10000011;
	while (bit_number_to_send < 7){
		//odd bit
		while (CHECK_SCK_HI){ //waiting for SCK to be in LOW state
			if (CHECK_CS_LO){ //IF CS gets LO then abort and set
				comm_stop(); //MISO and ACK goes LOW
				return -1;
			}
		}


		if (byte_out&(1<<bit_number_to_send)){
			SET_MISO_HI;
		}
		else{
			SET_MISO_LO;
		}
		SET_ACK_LO;
		bit_number_to_send++;
		

		//even bit
		while (CHECK_SCK_LO){ //waiting for SCK to be in HIGH state
			if (CHECK_CS_LO){
				comm_stop();//MISO and ACK goes LOW
				return -1;
			}
		}
		
		if (byte_out&(1<<bit_number_to_send)){
			SET_MISO_HI;
		}
		else{
			SET_MISO_LO;
		}

		SET_ACK_HI;
		bit_number_to_send++;
		

	}
	//PORTD=0b11100000;
	
	

	while (CHECK_MOSI_HI){ // !!! waiting for MOSI to be in LOW state after last bit
		//to not run away from function while master is not ready
		if (CHECK_CS_LO){ //IF CS gets LO then abort and set
			comm_stop(); //MISO and ACK goes LOW
			return -1;
		}
	}
	SET_MISO_LO;
	//comm_stop();	//MISO and ACK goes LOW
	
	while (CHECK_SCK_HI){ // !!! waiting for SCK to be in LOW state after last bit
		//to not run away from function while master is not ready
		if (CHECK_CS_LO){ //IF CS gets LO then abort and set
			comm_stop(); //MISO and ACK goes LOW
			return -1;
		}
	}

	SET_ACK_LO;
	//PORTD=0b11111000;
	return 0;
}

int send_device_id(){

	//string position
	int sp=0;
	//sends all chars of string.
	while (1){
		byte_out=device_id[sp];
		if (send_byte() == -1){//if error is returned then ends
			return -1;
		}
		sp++;
	}
	return 0;
	


}

int send_string(){
	//string position
	int sp=0;
	byte_out = 0;
	//sends all chars of string.
	while (byte_out != ';'){
		byte_out=teststring[sp];
		if (send_byte() == -1){//if error is returned then ends
			return -1;
		}
		sp++;
	}
	return 0;
}

int send_cmd_list(){
	//string position
	int sp=0;
	byte_out = 0;
	//sends all chars of string.
	while (byte_out != ';'){
		byte_out=command_list[sp];
		if (send_byte() == -1){//if error is returned then ends
			return -1;
		}
		sp++;
	}
	return 0;
}

int trap_status(){
	//string position
	int sp=0;
	byte_out = 0;

	if (CHECK_CLAMP_LO){
			//sends all chars of READY.
			while (byte_out != ';'){
				byte_out=status_ready[sp];
				if (send_byte() == -1){//if error is returned then ends
					return -1;
				}
				sp++;
			}
	}
	else {//sends all chars of CLAMP.
			while (byte_out != ';'){
				byte_out=status_clamp[sp];
				if (send_byte() == -1){//if error is returned then ends
					return -1;
				}
				sp++;
			}
	}

return 0;	
}

int toggle_beacon() {

	//string position
	int sp=0;
	byte_out = 0;

    if (beacon_status == 0){ // if BEACON off then ON and send string
		SET_LED_BEACON_HI;
		beacon_status = 1;

		while (byte_out != ';'){
			byte_out=beacon_on[sp];
			if (send_byte() == -1){//if error is returned then ends
				return -1;
			}
			sp++;
		}

	}
	else { // if BEACON on then OFF and send string
		SET_LED_BEACON_LO;
		beacon_status = 0;

		while (byte_out != ';'){
			byte_out=beacon_off[sp];
			if (send_byte() == -1){//if error is returned then ends
				return -1;
			}
			sp++;
		}
		
	}

return 0;						
}

int communication(){

	// RECEIVE BYTE

	switch (receive_byte()){

		case -1:
		return -1;


		case 1:
		if (send_device_id() == -1  ){//if error is returned then ends
			return -1;
		}
		break;
		
		case 2:
		if (send_cmd_list() == -1  ){//if error is returned then ends
			return -1;
		}
		break;
		
		case 10: //send string
		if (send_string() == -1){
			return -1;
		}
		break;
		
		case 11: //send string
		if (trap_status() == -1){
			return -1;
		}
		break;

		case 12: //toggle_beacon
		if (toggle_beacon() == -1){
			return -1;
		}
		break;
		
		case 254: // echo
		//receives second byte
		if (receive_byte() == -1  ){//if error is returned then ends
			return -1;

		}
		byte_out=byte_in;//prepares to send byte back
		//sends byte back
		if (send_byte() == -1  ){//if error is returned then ends
			return -1;
		}
		break;
		
		default:
		return 0;

	}

	return 0;
}




int main(void){

	port_init();

	while (1){

		if (CHECK_CS_HI){

			communication();
		}
	}

}