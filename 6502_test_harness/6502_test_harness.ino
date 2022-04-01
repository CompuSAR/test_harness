#include <setjmp.h>

                              //  0,  1,  2,  3,  4,  5,  6,  7
static const int DataBits[8] = {  4,  5,  6,  7,  8,  9, 10, 11 };
                                  //  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
static const int AddressBits[16] = { 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 32, 33, 34, 35, 36, 37 };
static const int RwBit = 38;
static const int SyncBit = 39;
static const int RMWBit = 40;
static const int ReadyInBit = 41;
static const int VectorPullBit = 42;

static const int ReadyOutBit = 43;
static const int NmiBit = 44;
static const int IrqBit = 45;
static const int BusEnableBit = 46;
static const int RamEnableBit = 47;
static const int ClockBit = 48;
static const int ResetBit = 49;
static const int SetOverflowBit = 50;
static const int CpuPowerBit = 51;

#include "operations.h"

#include <LibPrintf.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*(a+1)))

size_t cycleNum;
int clockState;

enum class IoState { Input, Read, Write } ioState;
void resetIo(IoState state) {
  if( ioState==state )
    return;

  bus( state==IoState::Input );

  if( ioState!=IoState::Input )
    digitalWrite(RamEnableBit, LOW); // Disable memory
  for( int i=0; i<ARRAY_SIZE(DataBits); ++i )
    pinMode(DataBits[i], state==IoState::Write ? OUTPUT : INPUT);

  for( int i=0; i<ARRAY_SIZE(AddressBits); ++i )
    pinMode(AddressBits[i], state==IoState::Input ? INPUT : OUTPUT);

  pinMode(RwBit, state==IoState::Input ? INPUT : OUTPUT);

  if( state==IoState::Input )
    digitalWrite(RamEnableBit, LOW); // Enable memory

  ioState = state;
}

void setup() {
  Serial.begin(1000000);

  pinMode(RwBit, OUTPUT);
  digitalWrite(RwBit, HIGH); // Disable write

  pinMode(BusEnableBit, OUTPUT);
  pinMode(CpuPowerBit, OUTPUT);
  cpu(true);

  ioState = IoState::Write;
  resetIo( IoState::Input );

  pinMode(SyncBit, INPUT);
  pinMode(ReadyInBit, INPUT);
  pinMode(VectorPullBit, INPUT);

  pinMode(ReadyOutBit, OUTPUT);
  digitalWrite(ReadyOutBit, HIGH);
  pinMode(SetOverflowBit, OUTPUT);
  digitalWrite(SetOverflowBit, HIGH);
  pinMode(NmiBit, OUTPUT);
  digitalWrite(NmiBit, HIGH);
  pinMode(IrqBit, OUTPUT);
  digitalWrite(IrqBit, HIGH);

  clockState = HIGH;
  pinMode(ClockBit, OUTPUT);
  digitalWrite(ClockBit, clockState);

  pinMode(ResetBit, OUTPUT);
  reset(HIGH);

  pinMode(RamEnableBit, OUTPUT);
  digitalWrite(RamEnableBit, LOW);

  cycleNum = 0;
}

void bus(bool value) {
    digitalWrite(BusEnableBit, value);
    printf("Bus %s\n", value ? "enabled" : "disabled");
}

unsigned collectBits( const int bits[], size_t numBits ) {
  unsigned result = 0;
  for(int i=0; i<numBits; ++i) {
    if( digitalRead(bits[i]) )
      result |= 1<<i;
  }

  return result;
}

#define COLLECT(a) (collectBits(a, ARRAY_SIZE(a)))
void dumpBus() {
  printf("%d: A:%04x ", cycleNum, COLLECT(AddressBits));
  if( clockState ) {
    byte data = COLLECT(DataBits);
    printf("D:%02x", data);

    if( digitalRead(SyncBit) ) {
      printf(" \"%s%s\"", OperationNames[ (int)opcodes[data].op ], AddressingModeNames[ (int)opcodes[data].mode ]);
    }
  } else {
    printf("D:--");
  }

  if( digitalRead(RwBit) )
    printf(" Read ");
  else
    printf(" Write");

  if( digitalRead(SyncBit) )
    printf(" SYNC");
  if( !digitalRead(ReadyInBit) )
    printf(" WAIT");
  if( !digitalRead(VectorPullBit) )
    printf(" VectorPull");
  if( !digitalRead(RMWBit) )
    printf(" MemoryLock");

  printf("\n");
}

bool resetState;
void reset(int value) {
  resetState = !value;
  printf("Reset %s\n", value ? "HIGH" : "LOW");
  digitalWrite(ResetBit, value);
}

void ready(int value) {
  printf("Ready %s\n", value ? "HIGH" : "LOW");
  digitalWrite(ReadyOutBit, value);
}

void setOverflow(int value) {
  printf("Set overflow %s\n", value ? "HIGH" : "LOW");
  digitalWrite(SetOverflowBit, value);
}

bool cpuState = false;
void cpu(bool value) {
  printf("CPU power %s\n", value ? "on" : "off");
  cpuState = value;
  resetIo( IoState::Input );
  digitalWrite(CpuPowerBit, cpuState);
}

static const char HELP_MSG[] =
"Command summary\n"
"B/b        Bus enable(b) disable(B)\n"
"C/c        Perform half(C) or full(c) clock cycle\n"
"d          Dump current bus state\n"
"I/i        Set the IRQ input low(I) or high(i)\n"
"N/n        Set the NMI input low(N) or high(n)\n"
"m aaaa     Read memory location from hex address aaaa\n"
"M aaaa,dd  Write hex value dd to address aaaa\n"
"P/p        Power the CPU off(P) or on(p) - not connected\n"
"R/r        Set reset line low(R) or high(r)\n"
"s          Single step\n"
"V/v        \"Set Overflow\" low(V) or high(v)\n"
"W/w        Set Ready line low(W) or high(w)\n"
"h/?        Print this message\n"
;

void loop() {
#define BUFFER_SIZE 120
  char commandLine[BUFFER_SIZE];
  int numBytes = readLine(commandLine, BUFFER_SIZE);
  if( numBytes==0 )
    return;

  switch(commandLine[0]) {
  case 'b':
    bus(true);
    break;
  case 'B':
    bus(false);
    break;
  case 'c': // Single clock
    advanceClock();
    break;
  case 'C': // Half a clock
    halfAdvanceClock();
    break;
  case 'd':
    resetIo( IoState::Input );
    dumpBus();
    break;
  case 'i': // IRQ
    digitalWrite( IrqBit, HIGH );
    printf("IRQ high\n");
    break;
  case 'I':
    digitalWrite( IrqBit, LOW );
    printf("IRQ low\n");
    break;
  case 'n': // NMI
    digitalWrite( NmiBit, HIGH );
    printf("NMI high\n");
    break;
  case 'N':
    digitalWrite( NmiBit, LOW );
    printf("NMI low\n");
    break;
  case 'm': // Memory read
    memoryReadCommand(commandLine);
    break;
  case 'M': // Memory write
    memoryWriteCommand(commandLine);
    break;
  case 'p': // Power on the CPU
    cpu(true);
    break;
  case 'P': // Power off the CPU
    cpu(false);
    break;
  case 'r': // Reset high (off)
    reset(HIGH);
    break;
  case 'R': // Reset low (on)
    reset(LOW);
    break;
  case 's': // Single step
    {
      int i=0;
      do {
        halfAdvanceClock();
      } while( clockState==LOW || (++i<20 && !digitalRead(SyncBit)) );
    }
    break;
  case 'v':
    setOverflow(HIGH);
    break;
  case 'V':
    setOverflow(LOW);
    break;
  case 'w':
    ready(HIGH);
    break;
  case 'W':
    ready(LOW);
    break;
  case 'h': // Help
  case '?':
    printHelp();
  }

}

void halfAdvanceClock() {
  resetIo( IoState::Input );

  clockState = !clockState;
  delay(1);
  digitalWrite(ClockBit, clockState);
  dumpBus();

  if( clockState )
    cycleNum++;
}

void advanceClock() {
  do {
    halfAdvanceClock();
  } while( clockState==LOW );
}

void memoryReadCommand(const char *commandLine) {
  uint16_t address;
  size_t index=1;
  jmp_buf errorBuf;

  if( setjmp(errorBuf)==0 ) {
    parseFixedChar( commandLine, index, ' ', errorBuf );
    address = parseFixedHex( commandLine, index, 4, errorBuf );
    parseVerifyEnd( commandLine, index, errorBuf );
  } else {
    printf("Memory read format: \"m fded\"\n");
    return;
  }

  resetIo( IoState::Read );
  digitalWrite(RwBit, HIGH); // Disable write
  digitalWrite(RamEnableBit, LOW); // Enable memory
  uint16_t mask=1;
  for( int i=0; i<ARRAY_SIZE(AddressBits); ++i ) {
    digitalWrite(AddressBits[i], (address & mask)!=0);
    mask <<= 1;
  }

  printf("%04x: %02x\n", address, COLLECT(DataBits));
}

void memoryWriteCommand(const char *commandLine) {
  uint16_t address;
  byte data;
  size_t index=1;
  jmp_buf errorBuf;

  if( setjmp(errorBuf)==0 ) {
    parseFixedChar( commandLine, index, ' ', errorBuf );
    address = parseFixedHex( commandLine, index, 4, errorBuf );
    parseFixedChar( commandLine, index, ',', errorBuf );
    data = parseFixedHex( commandLine, index, 2, errorBuf );
    parseVerifyEnd( commandLine, index, errorBuf );
  } else {
    printf("Memory write format: \"M fded,4d\"\n");
    return;
  }

  if( clockState==LOW ) {
    printf("Can't write to memory when the clock is low\n");
    return;
  }

  resetIo( IoState::Write );

  uint16_t mask=1;
  for( int i=0; i<ARRAY_SIZE(AddressBits); ++i ) {
    digitalWrite(AddressBits[i], (address & mask)!=0);
    mask <<= 1;
  }

  mask=1;
  for( int i=0; i<ARRAY_SIZE(DataBits); ++i ) {
    digitalWrite(DataBits[i], (data & mask)!=0);
    mask <<= 1;
  }

  digitalWrite(RwBit, LOW); // Enable write
  digitalWrite(RamEnableBit, LOW); // Enable memory

  printf("Written %02x to %04x\n", data, address);

  digitalWrite(RamEnableBit, HIGH); // Disable memory
  digitalWrite(RwBit, HIGH); // Disable write
}

static const char CRLF[]="\r\n";

size_t readLine(char *buffer, size_t bufferSize) {
  size_t len=0;

  while( len<bufferSize ) {
    if( Serial.available() ) {
      int ch = Serial.read();
      if( ch<0 )
        continue;

      if( ch=='\n' || ch=='\r' ) {
        Serial.write(CRLF);
        buffer[len]='\0';
        return len;
      } else {
        Serial.write(ch);
        buffer[len++] = ch;
      }
    }
  }

  Serial.write("\r\nLine overflow\r\n");
  return 0;
}

size_t parseFixedHex( const char *buffer, size_t &index, size_t numDigits, jmp_buf errorEnv ) {
  size_t result = 0;

  for( unsigned i=0; i<numDigits; ++i ) {
    char ch = buffer[index++];

    if( (ch>='0' and ch<='9') ) {
      ch -= '0';
    } else if(ch>='A' and ch<='F') {
      ch -= 'A';
      ch += 10;
    } else if(ch>='a' and ch<='f') {
      ch -= 'a';
      ch += 10;
    } else {
      longjmp(errorEnv, 1);
    }

    result *= 16;
    result += ch;
  }

  return result;
}

void parseFixedChar( const char *buffer, size_t &index, char expected, jmp_buf errorEnv ) {
  if( buffer[index++] != expected )
    longjmp(errorEnv, 1);
}

void parseVerifyEnd( const char *buffer, size_t &index, jmp_buf errorEnv ) {
  parseFixedChar( buffer, index, '\0', errorEnv );
}

void printHelp() {
  Serial.write(HELP_MSG);
}
