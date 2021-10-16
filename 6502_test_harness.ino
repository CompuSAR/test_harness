#include <setjmp.h>

                               // 0, 1, 2, 3, 4, 5, 6, 7
static const int DataBits[8] = { 2, 3, 4, 5, 6, 7, 8, 9 };
                                   //  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
static const int AddressBits[16] = { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 32, 33, 34, 35, 36, 37 };
static const int RwBit = 38;
static const int SyncBit = 39;
static const int RMWBit = 40;
static const int ReadyInBit = 41;
static const int VectorPullBit = 42;

static const int ReadyOutBit = 43;
static const int NmiBit = 44;
static const int IrqBit = 45;
static const int BusEnableBit = 46;
static const int ClockBit = 48;
static const int ResetBit = 49;
static const int CpuPowerBit = 51;

#include <LibPrintf.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*(a+1)))

size_t cycleNum;
int clockState;

void setup() {
  pinMode(CpuPowerBit, OUTPUT);
  cpu(false);
  
  for( int i=0; i<ARRAY_SIZE(DataBits); ++i )
    pinMode(DataBits[i], INPUT);

  for( int i=0; i<ARRAY_SIZE(AddressBits); ++i )
    pinMode(AddressBits[i], INPUT);

  pinMode(RwBit, INPUT);
  pinMode(SyncBit, INPUT);
  pinMode(ReadyInBit, INPUT);
  pinMode(VectorPullBit, INPUT);
  
  pinMode(ReadyOutBit, OUTPUT);
  digitalWrite(ReadyOutBit, HIGH);
  pinMode(NmiBit, OUTPUT);
  digitalWrite(NmiBit, HIGH);
  pinMode(IrqBit, OUTPUT);
  digitalWrite(IrqBit, HIGH);
  pinMode(BusEnableBit, OUTPUT);
  digitalWrite(BusEnableBit, HIGH);

  clockState = HIGH;
  pinMode(ClockBit, OUTPUT);
  digitalWrite(ClockBit, clockState);

  pinMode(ResetBit, OUTPUT);
  reset(LOW);

  Serial.begin(115200);

  cycleNum = 0;
}

void resetIo() {
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
  if( clockState )
    printf("D:%02x", COLLECT(DataBits));
  else
    printf("D:--");
  
  if( digitalRead(RwBit) )
    printf(" Read ");
  else
    printf(" Write");

  if( digitalRead(SyncBit) )
    printf(" SYNC");
  if( !digitalRead(ReadyInBit) )
    printf(" WAIT");
  /*
  if( !digitalRead(VectorPullBit) )
    printf(" VectorPull");
  */

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

bool cpuState = false;
void cpu(bool value) {
  printf("CPU power %s\n", value ? "on" : "off");
  cpuState = value;
  digitalWrite(CpuPowerBit, cpuState);
}

void loop() {
#define BUFFER_SIZE 120
  char commandLine[BUFFER_SIZE];
  int numBytes = readLine(commandLine, BUFFER_SIZE);
  if( numBytes==0 )
    return;

  switch(commandLine[0]) {
  case 'p': // Power on the CPU
    cpu(true);
    break;
  case 'P': // Power off the CPU
    cpu(false);
    break;
  case 's': // Single step
    if( resetState || !cpuState ) {
      printf("Can't single step when CPU is off or in reset\n");
      return;
    }
    do {
      halfAdvanceClock();
    } while( !digitalRead(SyncBit) );
    break;
  case 'c': // Single clock
    advanceClock();
    break;
  case 'C': // Half a clock
    halfAdvanceClock();
    break;
  case 'r': // Reset high (off)
    reset(HIGH);
    break;
  case 'R': // Reset low (on)
    reset(LOW);
    break;
  }
  
}

void halfAdvanceClock() {
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
