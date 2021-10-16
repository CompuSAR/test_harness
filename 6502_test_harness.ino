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
  digitalWrite(CpuPowerBit, LOW);
  
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
  digitalWrite(ResetBit, LOW);

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
  if( !digitalRead(VectorPullBit) )
    printf(" VectorPull");

  printf("\n");
}

void reset(int value) {
  printf("Reset %s\n", value ? "HIGH" : "LOW");
  digitalWrite(ResetBit, value);
}

void ready(int value) {
  printf("Ready %s\n", value ? "HIGH" : "LOW");
  digitalWrite(ReadyOutBit, value);
}
void loop() {
  if( cycleNum==10 ) {
    printf("Power on\n");
    digitalWrite(CpuPowerBit, HIGH);
  }
    
  if( cycleNum==15 ) {
    reset(HIGH);
  }

  /*
  if( cycleNum==20 )
    digitalWrite(ResetBit, LOW);
  if( cycleNum==25 )
    digitalWrite(ResetBit, HIGH);
  */

  if( cycleNum==35 )
    ready(LOW);
  if( cycleNum==45 )
    ready(HIGH);

  clockState = !clockState;
  digitalWrite(ClockBit, clockState);
  delay(500);
  dumpBus();

  if( clockState )
    cycleNum++;
}
