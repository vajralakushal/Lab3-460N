/*
    Name 1: Kushal Vajrala
    UTEID 1: kv7287
*/

/***************************************************************/
/*                                                             */
/*   LC-3b Simulator                                           */
/*                                                             */
/*   EE 460N                                                   */
/*   The University of Texas at Austin                         */
/*                                                             */
/***************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/***************************************************************/
/*                                                             */
/* Files:  ucode        Microprogram file                      */
/*         isaprogram   LC-3b machine language program file    */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/

void eval_micro_sequencer();
void cycle_memory();
void eval_bus_drivers();
void drive_bus();
void latch_datapath_values();

/***************************************************************/
/* A couple of useful definitions.                             */
/***************************************************************/
#define FALSE 0
#define TRUE  1

/***************************************************************/
/* Use this to avoid overflowing 16 bits on the bus.           */
/***************************************************************/
#define Low16bits(x) ((x) & 0xFFFF)

/***************************************************************/
/* Definition of the control store layout.                     */
/***************************************************************/
#define CONTROL_STORE_ROWS 64
#define INITIAL_STATE_NUMBER 18

/***************************************************************/
/* Definition of bit order in control store word.              */
/***************************************************************/
enum CS_BITS {                                                  
    IRD,
    COND1, COND0,
    J5, J4, J3, J2, J1, J0,
    LD_MAR,
    LD_MDR,
    LD_IR,
    LD_BEN,
    LD_REG,
    LD_CC,
    LD_PC,
    GATE_PC,
    GATE_MDR,
    GATE_ALU,
    GATE_MARMUX,
    GATE_SHF,
    PCMUX1, PCMUX0,
    DRMUX,
    SR1MUX,
    ADDR1MUX,
    ADDR2MUX1, ADDR2MUX0,
    MARMUX,
    ALUK1, ALUK0,
    MIO_EN,
    R_W,
    DATA_SIZE,
    LSHF1,
    CONTROL_STORE_BITS
} CS_BITS;

/***************************************************************/
/* Functions to get at the control bits.                       */
/***************************************************************/
int GetIRD(int *x)           { return(x[IRD]); }
int GetCOND(int *x)          { return((x[COND1] << 1) + x[COND0]); }
int GetJ(int *x)             { return((x[J5] << 5) + (x[J4] << 4) +
				      (x[J3] << 3) + (x[J2] << 2) +
				      (x[J1] << 1) + x[J0]); }
int GetLD_MAR(int *x)        { return(x[LD_MAR]); }
int GetLD_MDR(int *x)        { return(x[LD_MDR]); }
int GetLD_IR(int *x)         { return(x[LD_IR]); }
int GetLD_BEN(int *x)        { return(x[LD_BEN]); }
int GetLD_REG(int *x)        { return(x[LD_REG]); }
int GetLD_CC(int *x)         { return(x[LD_CC]); }
int GetLD_PC(int *x)         { return(x[LD_PC]); }
int GetGATE_PC(int *x)       { return(x[GATE_PC]); }
int GetGATE_MDR(int *x)      { return(x[GATE_MDR]); }
int GetGATE_ALU(int *x)      { return(x[GATE_ALU]); }
int GetGATE_MARMUX(int *x)   { return(x[GATE_MARMUX]); }
int GetGATE_SHF(int *x)      { return(x[GATE_SHF]); }
int GetPCMUX(int *x)         { return((x[PCMUX1] << 1) + x[PCMUX0]); }
int GetDRMUX(int *x)         { return(x[DRMUX]); }
int GetSR1MUX(int *x)        { return(x[SR1MUX]); }
int GetADDR1MUX(int *x)      { return(x[ADDR1MUX]); }
int GetADDR2MUX(int *x)      { return((x[ADDR2MUX1] << 1) + x[ADDR2MUX0]); }
int GetMARMUX(int *x)        { return(x[MARMUX]); }
int GetALUK(int *x)          { return((x[ALUK1] << 1) + x[ALUK0]); }
int GetMIO_EN(int *x)        { return(x[MIO_EN]); }
int GetR_W(int *x)           { return(x[R_W]); }
int GetDATA_SIZE(int *x)     { return(x[DATA_SIZE]); } 
int GetLSHF1(int *x)         { return(x[LSHF1]); }

/***************************************************************/
/* The control store rom.                                      */
/***************************************************************/
int CONTROL_STORE[CONTROL_STORE_ROWS][CONTROL_STORE_BITS];

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
   MEMORY[A][1] stores the most significant byte of word at word address A 
   There are two write enable signals, one for each byte. WE0 is used for 
   the least significant byte of a word. WE1 is used for the most significant 
   byte of a word. */

#define WORDS_IN_MEM    0x08000 
#define MEM_CYCLES      5
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/

/***************************************************************/

/***************************************************************/
/* LC-3b State info.                                           */
/***************************************************************/
#define LC_3b_REGS 8

int RUN_BIT;	/* run bit */
int BUS;	/* value of the bus */

typedef struct System_Latches_Struct{

int PC,		/* program counter */
    MDR,	/* memory data register */
    MAR,	/* memory address register */
    IR,		/* instruction register */
    N,		/* n condition bit */
    Z,		/* z condition bit */
    P,		/* p condition bit */
    BEN;        /* ben register */

int READY;	/* ready bit */
  /* The ready bit is also latched as you dont want the memory system to assert it 
     at a bad point in the cycle*/

int REGS[LC_3b_REGS]; /* register file. */

int MICROINSTRUCTION[CONTROL_STORE_BITS]; /* The microinstruction */

int STATE_NUMBER; /* Current State Number - Provided for debugging */ 
} System_Latches;

/* Data Structure for Latch */

System_Latches CURRENT_LATCHES, NEXT_LATCHES;

/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int CYCLE_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands.                   */
/*                                                             */
/***************************************************************/
void help() {                                                    
    printf("----------------LC-3bSIM Help-------------------------\n");
    printf("go               -  run program to completion       \n");
    printf("run n            -  execute program for n cycles    \n");
    printf("mdump low high   -  dump memory from low to high    \n");
    printf("rdump            -  dump the register & bus values  \n");
    printf("?                -  display this help menu          \n");
    printf("quit             -  exit the program                \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {                                                

  eval_micro_sequencer();   
  cycle_memory();
  eval_bus_drivers();
  drive_bus();
  latch_datapath_values();

  CURRENT_LATCHES = NEXT_LATCHES;

  CYCLE_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3b for n cycles.                 */
/*                                                             */
/***************************************************************/
void run(int num_cycles) {                                      
    int i;

    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating for %d cycles...\n\n", num_cycles);
    for (i = 0; i < num_cycles; i++) {
	if (CURRENT_LATCHES.PC == 0x0000) {
	    RUN_BIT = FALSE;
	    printf("Simulator halted\n\n");
	    break;
	}
	cycle();
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate the LC-3b until HALTed.                 */
/*                                                             */
/***************************************************************/
void go() {                                                     
    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating...\n\n");
    while (CURRENT_LATCHES.PC != 0x0000)
	cycle();
    RUN_BIT = FALSE;
    printf("Simulator halted\n\n");
}

/***************************************************************/ 
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a word-aligned region of memory to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void mdump(FILE * dumpsim_file, int start, int stop) {          
    int address; /* this is a byte address */

    printf("\nMemory content [0x%.4x..0x%.4x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	printf("  0x%.4x (%d) : 0x%.2x%.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    printf("\n");

    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%.4x..0x%.4x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	fprintf(dumpsim_file, " 0x%.4x (%d) : 0x%.2x%.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

/***************************************************************/
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump current register and bus values to the     */   
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump(FILE * dumpsim_file) {                               
    int k; 

    printf("\nCurrent register/bus values :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count  : %d\n", CYCLE_COUNT);
    printf("PC           : 0x%.4x\n", CURRENT_LATCHES.PC);
    printf("IR           : 0x%.4x\n", CURRENT_LATCHES.IR);
    printf("STATE_NUMBER : 0x%.4x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    printf("BUS          : 0x%.4x\n", BUS);
    printf("MDR          : 0x%.4x\n", CURRENT_LATCHES.MDR);
    printf("MAR          : 0x%.4x\n", CURRENT_LATCHES.MAR);
    printf("CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    printf("Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	printf("%d: 0x%.4x\n", k, CURRENT_LATCHES.REGS[k]);
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Cycle Count  : %d\n", CYCLE_COUNT);
    fprintf(dumpsim_file, "PC           : 0x%.4x\n", CURRENT_LATCHES.PC);
    fprintf(dumpsim_file, "IR           : 0x%.4x\n", CURRENT_LATCHES.IR);
    fprintf(dumpsim_file, "STATE_NUMBER : 0x%.4x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    fprintf(dumpsim_file, "BUS          : 0x%.4x\n", BUS);
    fprintf(dumpsim_file, "MDR          : 0x%.4x\n", CURRENT_LATCHES.MDR);
    fprintf(dumpsim_file, "MAR          : 0x%.4x\n", CURRENT_LATCHES.MAR);
    fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    fprintf(dumpsim_file, "Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	fprintf(dumpsim_file, "%d: 0x%.4x\n", k, CURRENT_LATCHES.REGS[k]);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

/***************************************************************/
/*                                                             */
/* Procedure : get_command                                     */
/*                                                             */
/* Purpose   : Read a command from standard input.             */  
/*                                                             */
/***************************************************************/
void get_command(FILE * dumpsim_file) {                         
    char buffer[20];
    int start, stop, cycles;

    printf("LC-3b-SIM> ");

    scanf("%s", buffer);
    printf("\n");

    switch(buffer[0]) {
    case 'G':
    case 'g':
	go();
	break;

    case 'M':
    case 'm':
	scanf("%i %i", &start, &stop);
	mdump(dumpsim_file, start, stop);
	break;

    case '?':
	help();
	break;
    case 'Q':
    case 'q':
	printf("Bye.\n");
	exit(0);

    case 'R':
    case 'r':
	if (buffer[1] == 'd' || buffer[1] == 'D')
	    rdump(dumpsim_file);
	else {
	    scanf("%d", &cycles);
	    run(cycles);
	}
	break;

    default:
	printf("Invalid Command\n");
	break;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : init_control_store                              */
/*                                                             */
/* Purpose   : Load microprogram into control store ROM        */ 
/*                                                             */
/***************************************************************/
void init_control_store(char *ucode_filename) {                 
    FILE *ucode;
    int i, j, index;
    char line[200];

    printf("Loading Control Store from file: %s\n", ucode_filename);

    /* Open the micro-code file. */
    if ((ucode = fopen(ucode_filename, "r")) == NULL) {
	printf("Error: Can't open micro-code file %s\n", ucode_filename);
	exit(-1);
    }

    /* Read a line for each row in the control store. */
    for(i = 0; i < CONTROL_STORE_ROWS; i++) {
	if (fscanf(ucode, "%[^\n]\n", line) == EOF) {
	    printf("Error: Too few lines (%d) in micro-code file: %s\n",
		   i, ucode_filename);
	    exit(-1);
	}

	/* Put in bits one at a time. */
	index = 0;

	for (j = 0; j < CONTROL_STORE_BITS; j++) {
	    /* Needs to find enough bits in line. */
	    if (line[index] == '\0') {
		printf("Error: Too few control bits in micro-code file: %s\nLine: %d\n",
		       ucode_filename, i);
		exit(-1);
	    }
	    if (line[index] != '0' && line[index] != '1') {
		printf("Error: Unknown value in micro-code file: %s\nLine: %d, Bit: %d\n",
		       ucode_filename, i, j);
		exit(-1);
	    }

	    /* Set the bit in the Control Store. */
	    CONTROL_STORE[i][j] = (line[index] == '0') ? 0:1;
	    index++;
	}

	/* Warn about extra bits in line. */
	if (line[index] != '\0')
	    printf("Warning: Extra bit(s) in control store file %s. Line: %d\n",
		   ucode_filename, i);
    }
    printf("\n");
}

/************************************************************/
/*                                                          */
/* Procedure : init_memory                                  */
/*                                                          */
/* Purpose   : Zero out the memory array                    */
/*                                                          */
/************************************************************/
void init_memory() {                                           
    int i;

    for (i=0; i < WORDS_IN_MEM; i++) {
	MEMORY[i][0] = 0;
	MEMORY[i][1] = 0;
    }
}

/**************************************************************/
/*                                                            */
/* Procedure : load_program                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void load_program(char *program_filename) {                   
    FILE * prog;
    int ii, word, program_base;

    /* Open program file. */
    prog = fopen(program_filename, "r");
    if (prog == NULL) {
	printf("Error: Can't open program file %s\n", program_filename);
	exit(-1);
    }

    /* Read in the program. */
    if (fscanf(prog, "%x\n", &word) != EOF)
	program_base = word >> 1;
    else {
	printf("Error: Program file is empty\n");
	exit(-1);
    }

    ii = 0;
    while (fscanf(prog, "%x\n", &word) != EOF) {
	/* Make sure it fits. */
	if (program_base + ii >= WORDS_IN_MEM) {
	    printf("Error: Program file %s is too long to fit in memory. %x\n",
		   program_filename, ii);
	    exit(-1);
	}

	/* Write the word to memory array. */
	MEMORY[program_base + ii][0] = word & 0x00FF;
	MEMORY[program_base + ii][1] = (word >> 8) & 0x00FF;
	ii++;
    }

    if (CURRENT_LATCHES.PC == 0) CURRENT_LATCHES.PC = (program_base << 1);

    printf("Read %d words from program into memory.\n\n", ii);
}

/***************************************************************/
/*                                                             */
/* Procedure : initialize                                      */
/*                                                             */
/* Purpose   : Load microprogram and machine language program  */ 
/*             and set up initial state of the machine.        */
/*                                                             */
/***************************************************************/
void initialize(char *ucode_filename, char *program_filename, int num_prog_files) { 
    int i;
    init_control_store(ucode_filename);

    init_memory();
    for ( i = 0; i < num_prog_files; i++ ) {
	load_program(program_filename);
	while(*program_filename++ != '\0');
    }
    CURRENT_LATCHES.Z = 1;
    CURRENT_LATCHES.STATE_NUMBER = INITIAL_STATE_NUMBER;
    memcpy(CURRENT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[INITIAL_STATE_NUMBER], sizeof(int)*CONTROL_STORE_BITS);

    NEXT_LATCHES = CURRENT_LATCHES;

    RUN_BIT = TRUE;
}

/***************************************************************/
/*                                                             */
/* Procedure : main                                            */
/*                                                             */
/***************************************************************/
int main(int argc, char *argv[]) {                              
    FILE * dumpsim_file;

    /* Error Checking */
    if (argc < 3) {
	printf("Error: usage: %s <micro_code_file> <program_file_1> <program_file_2> ...\n",
	       argv[0]);
	exit(1);
    }

    printf("LC-3b Simulator\n\n");

    initialize(argv[1], argv[2], argc - 2);

    if ( (dumpsim_file = fopen( "dumpsim", "w" )) == NULL ) {
	printf("Error: Can't open dumpsim file\n");
	exit(-1);
    }

    while (1)
	get_command(dumpsim_file);

}

/***************************************************************/
/* Do not modify the above code.
   You are allowed to use the following global variables in your
   code. These are defined above.

   CONTROL_STORE
   MEMORY
   BUS

   CURRENT_LATCHES
   NEXT_LATCHES

   You may define your own local/global variables and functions.
   You may use the functions to get at the control bits defined
   above.

   Begin your code here 	  			       */
/***************************************************************/

//Global Variables to keep track of stuff across functions
int DR;
int SR1;
int firstOperand; //value in SR1
int secondOperand; //from Piazza question @53. There isn't an SR2MUX explicitly identified in the control store
         //however, it's always in bits 2:0 in IR if IR[5] is enabled. This is either an SR2 or imm5
int ADDR1;
int ADDR2;
int MARMUX_VAL;
int ALU_VAL;
int MDR;
int SHF;


void eval_micro_sequencer() {

  /* 
   * Evaluate the address of the next state according to the 
   * micro sequencer logic. Latch the next microinstruction.
   */

    //Figure C.5 in the Appendix
    int ird = GetIRD(CURRENT_LATCHES.MICROINSTRUCTION);
    if(ird == 1){ // this is state 32, handle the transfer for the opcode
        int opcode = (CURRENT_LATCHES.IR & 0xF000) >> 12; //shift it over 12 bits just to get the front four digits that is the op code
        //important to note that the opcode given above is also the state number
        memcpy(NEXT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[opcode], sizeof(int)*CONTROL_STORE_BITS); //not sure why the * is there for the size, will ask TA. copied from line 522
        NEXT_LATCHES.STATE_NUMBER = opcode;
        return;
    }

    int cond0 = (GetCOND(CURRENT_LATCHES.MICROINSTRUCTION) & 0x01) >> 0;
    int cond1 = (GetCOND(CURRENT_LATCHES.MICROINSTRUCTION) & 0x02) >> 1;
    int r = CURRENT_LATCHES.READY;
    int ben = CURRENT_LATCHES.BEN;
    int ir11 = (CURRENT_LATCHES.IR & 0x0800) >> 11;
    int j0 = (GetJ(CURRENT_LATCHES.MICROINSTRUCTION) & 0x01) >> 0;
    int j1 = (GetJ(CURRENT_LATCHES.MICROINSTRUCTION) & 0x02) >> 1;
    int j2 = (GetJ(CURRENT_LATCHES.MICROINSTRUCTION) & 0x04) >> 2;
    //These do not need to be shifted bc they are being added and not taken into account by the and and or gates
    int j3 = (GetJ(CURRENT_LATCHES.MICROINSTRUCTION) & 0x08);
    int j4 = (GetJ(CURRENT_LATCHES.MICROINSTRUCTION) & 0x10);
    int j5 = (GetJ(CURRENT_LATCHES.MICROINSTRUCTION) & 0x20);

    //these are the or gates that lead into the MUX
    int addrMode = (cond0 & cond1 & ir11) | j0;
    int ready = (cond0 & (~cond1) & r) | j1;
    int branch = ((~cond0) & cond1 & ben) | j2;

    j0 = addrMode;
    j1 = ready << 1;
    j2 = branch << 2;

    int nextState = j0 + j1 + j2 + j3 + j4 + j5;

    //Get ready to transition to the next state
    memcpy(NEXT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[nextState], sizeof(int)*CONTROL_STORE_BITS); //not sure why the * is there for the size, will ask TA. copied from line 522
    NEXT_LATCHES.STATE_NUMBER = nextState;
    return;
}

int cycles_passed = 1;
void cycle_memory() {
 
  /* 
   * This function emulates memory and the WE logic. 
   * Keep track of which cycle of MEMEN we are dealing with.  
   * If fourth, we need to latch Ready bit at the end of 
   * cycle to prepare microsequencer for the fifth cycle.  
   */
    
    //CHECK MIO.EN and only if it's 1
    //set ready bit to zero, set cycle to += 1
    //if it's the fourth memory cycle, set the ready bit to one
    // if ready bit is one, and cycle = 5
        // check if it's read or write R.W, read is 0, write is 1
        // if write, check data size (if 0, then byte, if 1, then word) and then write into memory
        // if read, check data size and then put whatever is in MAR into MDR
        //READY bit = 0, cycle = 0
    
    //update GLOBAL variable for cycle count

    if(GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        if(cycles_passed == 4){
            NEXT_LATCHES.READY = 1;
            cycles_passed += 1;
        } else if(cycles_passed == 5 && CURRENT_LATCHES.READY == 1){
            int operation = GetR_W(CURRENT_LATCHES.MICROINSTRUCTION);
            if(operation == 1){// write to memory
                MEMORY[CURRENT_LATCHES.MAR/2][0] = (CURRENT_LATCHES.MDR & 0x00FF);
                MEMORY[CURRENT_LATCHES.MAR/2][1] = (CURRENT_LATCHES.MDR & 0xFF00) >> 8; 
            } else{// read
                //this is handled in latch datapath. NOTE: check to see if this works
            }
            cycles_passed = 1;
        }
        else{
            cycles_passed += 1;
        }
    }

}



void eval_bus_drivers() {

  /* 
   * Datapath routine emulating operations before driving the bus.
   * Evaluate the input of tristate drivers 
   *             Gate_MARMUX,
   *		 Gate_PC,
   *		 Gate_ALU,
   *		 Gate_SHF,
   *		 Gate_MDR.
   */

   //the inputs to the gates listed above are as follows:
   //DR, for LD.REG
   //SR, for ALU purposes. Need a second one of these, depending on if it's SR2 or imm5
   //ADDR1MUX
   //ADDR2MUX; these two are for the MARMUX or PCMUX
   //MARMUX_VAL
   //ALU, for the value of the ALU's output after computations
   //SHF, for the value of the SHF's that the GATESHF puts on the bus

    //The following values from Table C.1 in Appendix C
    if(GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 0){
        DR = (CURRENT_LATCHES.IR & 0x0E00) >> 9;
    } else{// can only be zero or one
        DR = 7;
    }

    //The following values from Table C.1 in Appendix C
    if(GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION) == 0){
        SR1 = (CURRENT_LATCHES.IR & 0x0E00) >> 9;
    } else{// can only be zero or one
        SR1 = (CURRENT_LATCHES.IR & 0x01C0) >> 6;
    }
    firstOperand = CURRENT_LATCHES.REGS[SR1];

    //calculating SR2 or imm5
    int operandType = (CURRENT_LATCHES.IR & 0x0020) >> 5; //isolating IR[5]
    if(operandType == 1){
        secondOperand = (CURRENT_LATCHES.IR & 0x001F);// >> 4;

        if(((secondOperand & 0x0010) >> 4) == 1){
            secondOperand = (secondOperand | 0xFFF0);
        }
    } else{ // it's 0, and thus it's the SR2
        secondOperand = (CURRENT_LATCHES.IR & 0x0007); //get the register value first
        secondOperand = CURRENT_LATCHES.REGS[secondOperand];
    }

    //need to handle potential two's complement scenarios;
    int negFirstBit = (firstOperand & 0x8000) >> 15;
    int negSecondBit = (secondOperand & 0x8000) >> 15;

    if(negFirstBit == 1){
        short res = firstOperand;
        firstOperand = (~res) + 1;
        firstOperand = -firstOperand;
    }
    if(negSecondBit == 1){
        short res = secondOperand;
        secondOperand = (~res) + 1;
        secondOperand = -secondOperand;
    }

    firstOperand = Low16bits(firstOperand);
    secondOperand = Low16bits(secondOperand);

    

    //Now, firstOperand and secondOperand have the information needed to conduct the ALU operations on

    //handling the ALUK part of things
    //Table C.1 from Appendix C
    int alukSelectionBits = GetALUK(CURRENT_LATCHES.MICROINSTRUCTION);
    if(alukSelectionBits == 0){ //ADD
        

        ALU_VAL = firstOperand + secondOperand;

        ALU_VAL = Low16bits(ALU_VAL);

    } else if(alukSelectionBits == 1){ //AND
        ALU_VAL = firstOperand & secondOperand;
    } else if(alukSelectionBits == 2){ //XOR
        int res = firstOperand ^ secondOperand;
        ALU_VAL = Low16bits(res);
    } else if(alukSelectionBits == 3){ //PASSA
        //TODO: what is this?
        //ChatGPT says that it's the SR1 just going through
        ALU_VAL = CURRENT_LATCHES.REGS[SR1];
    }

    ALU_VAL = Low16bits(ALU_VAL);


    //values from Table C.1 in Appendix C
    if(GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION) == 0){
        ADDR1 = CURRENT_LATCHES.PC;
    } else{
        ADDR1 = CURRENT_LATCHES.REGS[SR1];
    }

    int addr2muxVal = GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION);
    //the following values from table C.1 in appendix C
    if(addr2muxVal == 0){
        ADDR2 = 0;
    } else if(addr2muxVal == 1){
        int offset6 = CURRENT_LATCHES.IR & 0x003F; //getting the last six bits
        //TODO: sign extension: offset6 = 
        int neg6 = (CURRENT_LATCHES.IR & 0x0020) >> 5;
        if(neg6 == 1){
            offset6 = offset6 | 0xFFC0;
        }
        ADDR2 = offset6;
    } else if(addr2muxVal == 2){
        int offset9 = CURRENT_LATCHES.IR & 0x01FF; //getting the last nine bits
        //TODO: sign extension: offset6 = 
        int neg9 = (CURRENT_LATCHES.IR & 0x0100) >> 8;
        if(neg9 == 1){
            offset9 = offset9 | 0xFF00;
        }
        ADDR2 = offset9;
    } else if(addr2muxVal == 3){
        int offset11 = CURRENT_LATCHES.IR & 0x07FF; //getting the last eleven bits
        //TODO: sign extension: offset6 = 
        int neg11 = (CURRENT_LATCHES.IR & 0x0400) >> 10;
        if(neg11 == 1){
            offset11 = offset11 | 0xFC00;
        }
        ADDR2 = offset11;
    }
    if(GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        ADDR2 = ADDR2 << 1;
    }

    if(GetMARMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 0){
        //Table C.1, data values
        int marmuxVal = (CURRENT_LATCHES.IR & 0x00FF) << 1; //no need for particular zero extension, just and it with zeros at the end of the last 8 bits
        MARMUX_VAL = marmuxVal;
    } else{
        MARMUX_VAL = ADDR1 + ADDR2;
    }
    MARMUX_VAL = Low16bits(MARMUX_VAL);

    if(GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION) == 1){ //the whole word is being written, Table C.1
        MDR = CURRENT_LATCHES.MDR; //is the MDR being written? or the SR value
    } else{ //only one byte is being written, this depends on the MAR[0] value
        int mar0Flag = (CURRENT_LATCHES.MAR & 0x0001);
        if(mar0Flag == 1){ //Section C.5.2, bits [7:0]
            //time to get the sr[7:0]'sr[7:0]
            int bottomHalf = (CURRENT_LATCHES.MDR & 0x00FF);
            int topHalf = (CURRENT_LATCHES.MDR & 0x00FF) << 8;
            MDR = bottomHalf + topHalf;
        } else{ //bits [15:8]
            int bottomHalf = (CURRENT_LATCHES.MDR & 0xFF00) >> 8;
            int topHalf = (CURRENT_LATCHES.MDR & 0xFF00);
            MDR = bottomHalf + topHalf;
        }
    }
    MDR = Low16bits(MDR);

    //nothing in the control store explicity to check if it's a SHF, so gonna use the OPcode to
    //find what to put on the BUS for GateSHF in the next function
    int shfChecker = (CURRENT_LATCHES.IR & 0xF000) >> 12;
    if(shfChecker == 13){
        //it's the shf opcode
        int val = (CURRENT_LATCHES.IR & 0x000F);; //the value to shift by
        int shfCondition = (CURRENT_LATCHES.IR & 0x0030) >> 4;
        if(shfCondition == 0){//LSHF
            SHF = CURRENT_LATCHES.REGS[SR1] << val;
        } else if(shfCondition == 1){//RSHFL
            SHF = CURRENT_LATCHES.REGS[SR1] >> val;
        } else if(shfCondition == 3){//RSHFA
            int negFirstBit = (firstOperand & 0x8000) >> 15; //to check if the sign bits need to be shifted over as well
            //TODO: the firstOperand could be manipulated in the middle? idk
            if(negFirstBit == 1){
                // //first, two's complement it. It's easier to arithmetically right shift
                // short res = firstOperand;
                // firstOperand = (~res) + 1;
                // firstOperand = -firstOperand;
                // firstOperand = firstOperand >> val; //in C, this should arithmetic right shift
                // //now, we can two's complement it again
                // firstOperand = ~firstOperand;
                // firstOperand += 1;
                // SHF = firstOperand;
                firstOperand = firstOperand >> val;

                short mask = 0;
                int start = 15;
                int temp = val;
                while(temp >= 1){
                    int mul = 1;
                    for(int i = 0; i < start; i++){
                        mul *= 2;
                    }
                    mask += mul;
                    temp -= 1;
                    start -= 1;
                }
                SHF = firstOperand | mask;
                
            } else{// just a regular RSHFL again
                SHF = CURRENT_LATCHES.REGS[SR1] >> val;
            }

        }
    }
    SHF = Low16bits(SHF);
}


void drive_bus() {
  /* 
   * Datapath routine for driving the bus from one of the 5 possible 
   * tristate drivers. 
   */
    if(GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        //fill in bus value
        BUS = CURRENT_LATCHES.PC;
        return;
    }
    if(GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        BUS = MDR;
        return;
    }
    if(GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        BUS = ALU_VAL;
        return;
    }
    if(GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        BUS = MARMUX_VAL;
        return;
    }
    if(GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        BUS = SHF;
        return;
    }


    

}


void latch_datapath_values() {

  /* 
   * Datapath routine for computing all functions that need to latch
   * values in the data path at the end of this cycle.  Some values
   * require sourcing the bus; therefore, this routine has to come 
   * after drive_bus.
   */

   //make sure that for all of the following load instructions, if the value in current latches is not being changed,
   //explicitly keep the same value. We don't wanna hold onto the old values in NEXT_LATCHES jic

    if(GetLD_MAR(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        NEXT_LATCHES.MAR = BUS; //from Data Path in Appendix C. The MAR is loaded right off the BUS, but only 16 bits are loaded into the MAR
    } else{
        NEXT_LATCHES.MAR = CURRENT_LATCHES.MAR;
    }

    if(GetLD_MDR(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        //for the mux right before the LD.MDR on the data path. We have to see if MIO.EN is enabled along with
        //if the ready bit is enabled, as will be defined in cycle_memory
        //it'll load from MAR. Otherwise, it'll load from the BUS.
        if(CURRENT_LATCHES.READY == 1 && GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION) == 1){ 
            NEXT_LATCHES.MDR = MEMORY[CURRENT_LATCHES.MAR/2][0] + (MEMORY[CURRENT_LATCHES.MAR/2][1] << 8);
            NEXT_LATCHES.READY = 0;//does this work?
        } else{// i think we can just load off the BUS?
            NEXT_LATCHES.MDR = BUS;
        }
    } else{
        NEXT_LATCHES.MDR = CURRENT_LATCHES.MDR;
    }

    if(GetLD_IR(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        NEXT_LATCHES.IR = BUS; //from Data Path in Appendix C. The IR is loaded right off the BUS, but only 16 bits are loaded into the IR
    } else{
        NEXT_LATCHES.IR = CURRENT_LATCHES.IR;
    }

    if(GetLD_BEN(CURRENT_LATCHES.MICROINSTRUCTION) == 1){ //state 32 on the state diagram
        int ir11 = (CURRENT_LATCHES.IR & 0x0800) >> 11;
        int ir10 = (CURRENT_LATCHES.IR & 0x0400) >> 10;
        int ir9 = (CURRENT_LATCHES.IR & 0x0200) >> 9;
        int ben_load_val = (ir11 & CURRENT_LATCHES.N) + (ir10 & CURRENT_LATCHES.Z) + (ir9 & CURRENT_LATCHES.P);
        NEXT_LATCHES.BEN = ben_load_val;
    } else{
        NEXT_LATCHES.BEN = CURRENT_LATCHES.BEN;
    }


    memcpy(NEXT_LATCHES.REGS, CURRENT_LATCHES.REGS, sizeof(int)* 8);
    if(GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        NEXT_LATCHES.REGS[DR] = BUS;
    }

    if(GetLD_CC(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        //first, reset all of the condition codes. only one of them can be set
        NEXT_LATCHES.N = 0;
        NEXT_LATCHES.Z = 0;
        NEXT_LATCHES.P = 0;
        int val = BUS; //pulling value off the bus to see what it should be

        int neg_bit = (val & 0x8000) >> 15; //it's a four bit number, so the 1 at the MSB tells if it's negative or not
        if(neg_bit == 1){
            val = ~val; //google searched. this flips all of the bits
            val += 1; //converted from two's complement
        }
        val = Low16bits(val); //TODO: why does this work

        if(val < 0){
            NEXT_LATCHES.N = 1;
        } else if(val == 0){
            NEXT_LATCHES.Z = 1;
        } else{ //the only other option
            NEXT_LATCHES.P = 1;
        }
    } else{
        NEXT_LATCHES.N = CURRENT_LATCHES.N;
        NEXT_LATCHES.Z = CURRENT_LATCHES.Z;
        NEXT_LATCHES.P = CURRENT_LATCHES.P;
    }

    //Table C.1 from Appendix C
    if(GetLD_PC(CURRENT_LATCHES.MICROINSTRUCTION) == 1){
        int pcmux = GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION);
        if(pcmux == 0){//PC = PC + 2
            NEXT_LATCHES.PC = CURRENT_LATCHES.PC + 2;
        } else if(pcmux == 1){//select value from BUS
            NEXT_LATCHES.PC = BUS;
        } else if(pcmux == 2){//adder
            NEXT_LATCHES.PC = ADDR1 + ADDR2;
        }
    } else{
        NEXT_LATCHES.PC = CURRENT_LATCHES.PC;
    }        

}
