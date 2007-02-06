#ifndef _CODESEND_H_INCL_
#define _CODESEND_H_INCL_

typedef enum {
  DVM_CAPSULE_REBOOT	 = 0, 
  DVM_CAPSULE_ID1	 = 1, 
  DVM_CAPSULE_ID2	 = 2, 
  DVM_CAPSULE_ID3	 = 3, 
  DVM_CAPSULE_ID4	 = 4, 
  DVM_CAPSULE_ID5	 = 5, 
  DVM_CAPSULE_ID6	 = 6, 
  DVM_CAPSULE_ID7	 = 7, 
  DVM_CAPSULE_INVALID	 = 255
} DvmCapsuleID;

#ifndef PHOTO
#define PHOTO 0
#endif
#ifndef TEMPERATURE
#define TEMPERATURE 1
#endif 
#ifndef MIC
#define MIC 2
#endif
#ifndef ACCELX
#define ACCELX 3
#endif
#ifndef ACCELY
#define ACCELY 4
#endif
#ifndef MAGX
#define MAGX 5
#endif
#ifndef MAGY
#define MAGY 6
#endif

#define BASICLIB_MIN_OPCODE	0 
#define EXT_LIB_OPCODE_OFFSET	0x80
#define EXTLIB0_MIN_OPCODE 	(EXT_LIB_OPCODE_OFFSET + 0x00)
#define EXTLIB1_MIN_OPCODE 	(EXT_LIB_OPCODE_OFFSET + 0x20)
#define EXTLIB2_MIN_OPCODE 	(EXT_LIB_OPCODE_OFFSET + 0x40)
#define EXTLIB3_MIN_OPCODE 	(EXT_LIB_OPCODE_OFFSET + 0x60)

#define FLOAT_PRECISION	100L

typedef enum { // instruction set
  OP_HALT = (BASICLIB_MIN_OPCODE + 0),
  OP_PUSH = (BASICLIB_MIN_OPCODE + 1),
  OP_POP = (BASICLIB_MIN_OPCODE + 2),
  OP_LED = (BASICLIB_MIN_OPCODE + 3),
  OP_GETLOCAL = (BASICLIB_MIN_OPCODE + 4),	// 6 local variables
  OP_BSET = (BASICLIB_MIN_OPCODE + 10),
  OP_JMP = (BASICLIB_MIN_OPCODE + 11),
  OP_RAND = (BASICLIB_MIN_OPCODE + 12),
  OP_CALL = (BASICLIB_MIN_OPCODE + 13),	
  OP_POST = (BASICLIB_MIN_OPCODE + 14),	
  OP_POSTNET = (BASICLIB_MIN_OPCODE + 15),		
  OP_JZ = (BASICLIB_MIN_OPCODE + 16),		
  OP_BCAST = (BASICLIB_MIN_OPCODE + 17),
  OP_BREADF = (BASICLIB_MIN_OPCODE + 18),
  OP_BCLEAR = (BASICLIB_MIN_OPCODE + 19),	
  OP_DECR = (BASICLIB_MIN_OPCODE + 20),
  OP_SETTIMER = (BASICLIB_MIN_OPCODE + 21),	//21-28, 8 timers
  OP_ADD = (BASICLIB_MIN_OPCODE + 29),
  OP_MOD = (BASICLIB_MIN_OPCODE + 30),
  OP_INCR = (BASICLIB_MIN_OPCODE + 31),
  OP_SUB = (BASICLIB_MIN_OPCODE + 32),
  OP_MUL = (BASICLIB_MIN_OPCODE + 33),
  OP_DIV = (BASICLIB_MIN_OPCODE + 34),
  OP_GET_DATA = (BASICLIB_MIN_OPCODE + 35), //35-41, 7 sensors
  OP_JGE = (BASICLIB_MIN_OPCODE + 42),
  OP_JG = (BASICLIB_MIN_OPCODE + 43),
  OP_JLE = (BASICLIB_MIN_OPCODE + 44),
  OP_JL = (BASICLIB_MIN_OPCODE + 45),
  OP_JE = (BASICLIB_MIN_OPCODE + 46),
  OP_JNE = (BASICLIB_MIN_OPCODE + 47),
  OP_JNZ = (BASICLIB_MIN_OPCODE + 48),
  OP_GETVAR = (BASICLIB_MIN_OPCODE + 49),	//49-56, 8 shared vars
  OP_SETVAR = (BASICLIB_MIN_OPCODE + 57),	//57-64
  OP_BPUSH = (BASICLIB_MIN_OPCODE + 65),	//65-68, 4 buffers
  OP_PUSHF = (BASICLIB_MIN_OPCODE + 69),	// PUSHF 00 00 30 00  == 0.30
  OP_SETVARF = (BASICLIB_MIN_OPCODE + 70),	// 70-76, 7 slots for float var
  OP_GETVARF = (BASICLIB_MIN_OPCODE + 77),	// 77-84, 8 slots
  OP_BAPPEND = (BASICLIB_MIN_OPCODE + 85),	// 85-91, 7 arguments that can be appended to a buffer	
  OP_GETLOCALF = (BASICLIB_MIN_OPCODE + 92),	// 92-96, 5 slots
  OP_SETLOCALF = (BASICLIB_MIN_OPCODE + 97),	// 97-100, 4 slots
  OP_SETLOCAL = (BASICLIB_MIN_OPCODE + 101),	// 101-106, 6 slots
  OP_ABS = (BASICLIB_MIN_OPCODE + 107),
  OP_BZERO = (BASICLIB_MIN_OPCODE + 108),
  OP_BINIT = (BASICLIB_MIN_OPCODE + 109),
  OP_START = (BASICLIB_MIN_OPCODE + 110),
  OP_STOP = (BASICLIB_MIN_OPCODE + 111),
  OP_NOP = (BASICLIB_MIN_OPCODE + 112),

  OP_EWMA = (EXTLIB0_MIN_OPCODE + 0),
  OP_EWMADIR = (EXTLIB1_MIN_OPCODE + 0),

  OP_OUTLIER = (EXTLIB0_MIN_OPCODE + 0),
  OP_OUTDIR = (EXTLIB1_MIN_OPCODE + 0),
  
  OP_MOVE = (EXTLIB1_MIN_OPCODE + 0),
  OP_TURN = (EXTLIB1_MIN_OPCODE + 1),
  OP_PAN = (EXTLIB1_MIN_OPCODE + 2),
  OP_TILT = (EXTLIB1_MIN_OPCODE + 3),
  OP_SCAN = (EXTLIB1_MIN_OPCODE + 4),
  OP_SHOW_VALUE = (EXTLIB1_MIN_OPCODE + 5),
  OP_FIND_DISTANCE = (EXTLIB1_MIN_OPCODE + 6),
  OP_SALUTE = (EXTLIB1_MIN_OPCODE + 7),
  OP_FLASHLED = (EXTLIB1_MIN_OPCODE + 8),
  OP_ENABLE_CLIFF = (EXTLIB1_MIN_OPCODE + 9),
  OP_ENABLE_PUSHBUTTON = (EXTLIB1_MIN_OPCODE + 10),
} DvmInstruction;

#endif