#include "can.h"
#include "spi.h"
#include "settings.h"
#include <util/delay.h>

void CAN_init(void){

	CAN_reset();

	CAN_bit_modify(CANCTRL, MASK_MODE, MODE_LOOPBACK); //set loopback mode
	CAN_bit_modify(RXB0CTRL, MASK_RECEIVE_ID_TYPE, ID_TYPE_STANDARD); // set no filter, set to 01 to accept only standard, 00 to accept accordig to filters
		
}

int CAN_test(void){
	char received[9];
	char read[9];
	read[8] = '\0';
	int i;
	
	for (i = 0;(i < 9); i++)
		received[i] = '\0';	


	if (CAN_send(0, "aaaaaaaa", 8) != 0){
		return -1;
	}

	_delay_ms(100);
		
	CAN_receive(received, 0); //rxbuffer 0

	for (i = 0;(i < 9); i++)
		printf("0x%x ", (uint8_t)received[i]);
	printf("\n");

	return 0;
}

int CAN_send(int id, char* data, int n){
	
	unsigned int i;
	CAN_bit_modify(TXB0SIDH, 0xFF, (id<<13)); //transmit buffer 0 id high
	CAN_bit_modify(TXB0SIDL, MASK_SIDL, (id<<5));//transmit buffer 0 id low
	CAN_write((char *)&n, TXB0DLC, 1);	// data length (as char pointer)
	CAN_load_tx(data, 0, n); //load transmit buffer
	CAN_rts(0); //request to send
	

	//wait for send OK ()
	for(i = 0; i < 0xffff; i++){
		if((CAN_read_status() & MASK_TXREQ0) == 0) break;
	}
	if(i == 0xffff) return -1;
	return 0;
}

int CAN_receive(char * data, int rx){
	//FILHIT for � sjekke type beskjed
	
	while((CAN_read_status() & MASK_CANINTF_RX0IF) == 0); // loop until data received
	CAN_read_rx(data, rx, 8);
	
	return 0;

}



void CAN_reset(void){
	SPI_SelectSlave(SPI_CAN);
	SPI_MasterTransmit(INS_RESET);
	SPI_NoSlave();
}

void CAN_read(char* data, uint8_t address , int data_count){
	int i;
	SPI_SelectSlave(SPI_CAN);	

	SPI_MasterTransmit(INS_READ);
	SPI_MasterTransmit((char)address);
	for(i = 0; i < data_count; i++){
		data[i] = SPI_MasterReceive();
	}

	SPI_NoSlave();

}

void CAN_read_rx(char* data, uint8_t rx, int data_count){
	int i;
	if (rx>1 || data_count > 8)
		return;
	if(rx == 0) rx = 1; //decode rx0 to word for "read from rxb0", standard frame
	else if(rx == 1) rx = 3; //decode rx1 to intruction for "read from rxb1", standard frame
	
	SPI_SelectSlave(SPI_CAN);	
	SPI_MasterTransmit(INS_READ_RX | (rx<<1));
	for (i = 0; i < data_count; i++){
		data[i] = SPI_MasterReceive();
	}
	
	SPI_NoSlave();
}

void CAN_write(char* data, uint8_t address, int data_count){
	int i;
	SPI_SelectSlave(SPI_CAN);	

	SPI_MasterTransmit(INS_WRITE);
	SPI_MasterTransmit((char)address);
	for(i = 0; i < data_count; i++){
		SPI_MasterTransmit(data[i]);
	}

	SPI_NoSlave();

}
//tx = "modul" (3 output "kanaler")
void CAN_load_tx(char* data, uint8_t tx, int data_count){
	int i;
	if (tx>2 || data_count > 8)
		return;
	tx = (tx+1)*2 - 1; //convert to abc-format as explained in table 12-5
	SPI_SelectSlave(SPI_CAN);
	
	SPI_MasterTransmit(INS_LOAD_TX | tx);
	for(i = 0; i < data_count; i++){
		printf("%c", data[i]);
		SPI_MasterTransmit(data[i]);
	}

	SPI_NoSlave();
	printf("\n");
}

void CAN_rts(uint8_t tx){
	if (tx == 0) tx = 1;
	else if (tx == 1) tx = 2;
	else if (tx == 2) tx = 4;
	else return;
	
	SPI_SelectSlave(SPI_CAN);
	printf("Rts: 0x%x\n", (INS_RTS | tx));
	SPI_MasterTransmit(INS_RTS | tx);

	SPI_NoSlave();
}

uint8_t CAN_read_status(void){
	char status;
	SPI_SelectSlave(SPI_CAN);

	SPI_MasterTransmit(INS_READ_STATUS);
	status = SPI_MasterReceive();

	SPI_NoSlave();
	
	return (uint8_t) status;

}

uint8_t CAN_rx_status(void){
return 0;

}
void CAN_bit_modify(uint8_t address, uint8_t mask, uint8_t data){
	SPI_SelectSlave(SPI_CAN);


	SPI_MasterTransmit((char)INS_BIT_MODIFY);	
	SPI_MasterTransmit((char)address);
	SPI_MasterTransmit((char)mask);
	SPI_MasterTransmit((char)data);

	SPI_NoSlave();
}
