// some defines that determine which tests are included
//#define TEST_EVENT
//#define TEST_TIMER
#define TEST_DIGITAL

#if defined AVR // run on Arduino
#include "Arduino.h"
#include "HardwareSerial.h"


//typedef unsigned long ulong; // unsigned long int gets a bit tedious

#define co(x) Serial.print(x);
#define coln(x) Serial.println(x)

void addMillis(uLong m) {
    delay(m);
}

void sleep(int x) {
    delay(x*1000);
}

// static const int sleepTime=1;

#else // debug on Linux, OSX, other PC
#include <stdio.h>
#include <iostream>
#include <unistd.h>
using namespace std;

typedef unsigned long uLong; // unsigned long int gets a bit tedious

// co => console output
#define co(x) cout << (x)
#define coln(x) {cout << (x) << endl;}

// static const int sleepTime=0;

// g++ -Wall -DDEBUG  -o testEF testEF.cpp

// now stub out some Arduino calls

uLong millisVal=0;
void addMillis(uLong m) {
    millisVal += m;
}
unsigned long millis() {
    return millisVal;
}
void delay(unsigned int n) {
    millisVal+=n;
}

static const int INPUT=1;
void pinMode(int pin, int direction)
{
    return;
}

struct { int pin; bool val; } IOmap[] = {
	{  0, false }, // RX, serial I/O - don't use
	{  0, false }, // TX - don't use
	{  0, false },
	{  0, false },
	{  5, false },		// pin 4 mapped to 5 // writing pin 4 sets pin 5
	{  0, false },
	{  7, false },		// pin 6 mapped to 7
	{  0, false },
	{ 14, false },		// pin 8 mapped to A0
	{  0, false },
	{  0, false },
	{  0, false },
	{  0, false },
	{  0, false },		// A0 masquerading as digital input and mapped to pin 10
	{  0, false },
	{  0, false },
	{  0, false },
	{  0, false },
	{  0, false },		// last digital bit on Uno
};

//static bool pinVals[20]; // number of digital input pins (must match size of testMap)
bool digitalRead(unsigned int p) {
    return IOmap[p].val;
}

#define X(x) x
#define F(x) x
#define Printf printf

void digitalWrite(int pin, unsigned char value) {
    if (pin < sizeof(IOmap)/sizeof(IOmap[0]))
	IOmap[IOmap[pin].pin].val = value;
    Printf(F("wrote %d to bit %d mapped to %d\n"), value, pin, IOmap[pin].pin);
}


#endif // defined AVR 

// #define NDEBUG  uncomment to disable asserts.
#include <assert.h>
//#include <algorithm>    // std::min

static bool verbose=false;      // turn on/off verbose crap. Make it const and the compiler is free to optimize away the code

#include "EventFramework.h"

#if defined TEST_TIMER
class MyTimer:
    public efl::Timer  // periodic timer by default
{
private:
    virtual bool callback(uLong late) {
        callCount++;
        /* if (verbose || late) {
         co( "MyTimer: ");
         coln(id);
         } */
        return true;
    };
    int     id;
    int     callCount;
public:
    MyTimer(int i, uLong c=1, uLong p=1):
        efl::Timer(c,p),id(i),callCount(0) {
    };
    int getCallCount() {
        return callCount;
    };
    int clearCallCount() {
        int rc=callCount;
        callCount=0;
        return rc;
    };
};

class MyOneShotTimer:
    public efl::Timer
{
private:
    virtual bool callback(uLong late)
    {
        callCount++;
        if (verbose) {
            co( X("MyOneShotTimer: "));
            coln(id);
        }
        return retVal;
    };
    int     id;
    int     callCount;
    bool    retVal;
public:
    MyOneShotTimer(int i, unsigned long c):
        efl::Timer(c,0),id(i),callCount(0),retVal(false) {
    };
    int getCallCount() {
        return callCount;
    };
    int clearCallCount() {
        int rc=callCount;
        callCount=0;
        return rc;
    };
    void setRepeat(bool r, uLong p) {
        retVal=r;    // enforce consistency in args???
        setPeriod(p);
    }
};
#endif //defined TEST_TIMER

#if defined TEST_EVENT
// derive my event from the generic one so I can provide my own callback.
class MyEvent :
    public efl::Event {
protected:
    int     id;
public:
    MyEvent(int n):
        id(n) {
    };
    virtual bool callback() {
        if (verbose) {
            co( "MyEvent:");
            coln( id);
        }
        return false;
    };
};

class RepeatEvent:
    public MyEvent  // this event remains in the queue and will execute each time Event list is examined
{
public:
    RepeatEvent(int n):
        MyEvent(n) {
    };
    virtual bool callback() {
        if (verbose) {
            co( "RepeatEvent:");
            coln( id);
        }
        return true;
    };
};
#endif //defined TEST_EVENT

#if defined TEST_DIGITAL
class MyDigital:
    public efl::Digital  //
{
private:
    virtual bool callback(uLong late, States newstate, States oldState) {
        callCount++;
        if (verbose || late) {
	    Printf(X("Digital::callback( late:%lu state:%d oldst:%d\n"), late, newstate, oldState );
            coln(id);
        }
        return true;
    };
    int     id;
    int     callCount;
public:
    MyDigital(int i, DigitalBit b, int d=1, Polarity p = ACT_HI, efl::uchar interest = (INACTIVE|ACTIVE)):
        efl::Digital(b,d, p, interest),id(i),callCount(0) {
    };
    int getCallCount() {
        return callCount;
    };
    int clearCallCount() {
        int rc=callCount;
        callCount=0;
        return rc;
    };
};
#endif //defined TEST_DIGITAL


#if defined AVR
void setup()
{
    Serial.begin(115200);
}

void loop()
{
#else
int main()
{
#endif

#if defined TEST_EVENT
    MyEvent         e1(1);
    MyEvent         e2(2);
    MyEvent         e3(3);
    RepeatEvent     re1(4);

    efl::LL<efl::Event>       le1(&e1);
    efl::LL<efl::Event>       le2(&e2);
    efl::LL<efl::Event>       le3(&e3);
    efl::LL<efl::Event>       lre1(&re1);

    efl::LL<efl::Event>*  pScratchE;


    /* codify the regression tests */

    coln( "efl::LL<efl::Event> tests");


    delay(10000);
    co( "erase() w/o add() ..............................................");
    if( le1.erase() == efl::LL<efl::Event>::begin() && le1.size() == 0 )
    {
        coln( "OK" );
    }
    else
    {
        coln("FAILED "); /* co( le1.erase() ); co( " " ); co(  efl::LL<efl::Event>::begin() ); co( " " ); coln(le1.size());*/
    }

    co( "add() ..........................................................");
    if( le1.add() ==  efl::LL<efl::Event>::OK && (pScratchE = efl::LL<efl::Event>::begin()) == &le1 && le1.size() == 1
            && pScratchE->next() == efl::LL<efl::Event>::end() )
    {
        coln( "OK" );
    }
    else
    {
        coln( "FAILED " );
    }

    co( "erase().........................................................");
    if( le1.erase() == efl::LL<efl::Event>::end() && le1.size() == 0 )
    {
        coln( "OK" );
    }
    else
    {
        coln( "FAILED " );
    }

    le1.add();
    co( "add() /dup......................................................");
    if( le1.add() == efl::LL<efl::Event>::BAD_DUP && efl::LL<efl::Event>::size() == 1  )
    {
        coln( "OK" );
    }
    else
    {
        coln( "FAILED " );
    }

    le2.push();

    co( "erase()/first...................................................");
    if( (pScratchE=le2.erase()) == &le1 && le1.begin() == &le1 && le1.size() == 1 )
    {
        coln("OK");
    }
    else
    {
        coln("FAILED");
    }

    le2.push();
    co( "erase()/second..................................................");
    if( (pScratchE=le1.erase()) == le1.end() && le1.begin() == &le2 && le1.size() == 1 )
    {
        coln("OK");
    }
    else
    {
        coln("FAILED");
    }

    co( "Adding 1,3 and removing the first...............................");
    if( le1.add() == efl::LL<efl::Event>::OK && le3.add() == efl::LL<efl::Event>::OK  && le2.erase() == le2.begin() && le2.size() == 2 )
    {
        coln("OK");
    }
    else
    {
        coln("FAILED");
    }

    co( "Adding 2 to front and removing middle...........................");
    if( le2.add() == efl::LL<efl::Event>::OK && le3.erase() == le2.begin()->next() && le2.size() == 2 )
    {
        coln("OK");
    }
    else
    {
        coln("FAILED");
    }

    co( "Removing last and then only remaining Item......................");
    if( le1.erase() == &le2 && le2.erase() == le2.end() && le2.size() == 0 )
    {
        coln("OK");
    }
    else
    {
        coln("FAILED");
    }

    co( "Add and remove and try to remove sentinel.......................");
    if( le1.add() ==  efl::LL<efl::Event>::OK && le1.erase() == le1.end() && le2.size() == 0
            && le1.end()->erase() == le1.end() && le2.size() == 0 )
    {
        coln("OK");
    }
    else
    {
        coln("FAILED");
    }

    co( "Try to add sentinel.............................................");
    if( le1.end()->add() == efl::LL<efl::Event>::BAD_DUP && le2.size() == 0 )
    {
        coln("OK");
    }
    else
    {
        coln("FAILED");
    }

    co( "Try to remove sentinel..........................................");
    if( le1.end()->erase() ==  efl::LL<efl::Event>::end() && le2.size() == 0 )
    {
        coln("OK");
    }
    else
    {
        coln("FAILED");
    }


    co( "Check begin(), end(), previous() and next() on empty list.......");
    bool result=true;   // set default to true
    if( !(efl::LL<efl::Event>::end() ==  efl::LL<efl::Event>::begin() && le2.size() == 0 ))
        result = false;
    if( efl::LL<efl::Event>::begin()->next() != efl::LL<efl::Event>::end())
        result = false;
    if( efl::LL<efl::Event>::begin()->previous() != efl::LL<efl::Event>::end())
        result = false;

    if(result)
    {
        coln("OK");
    }
    else
    {
        coln("FAILED");
    }

    co( "Check begin(), end(), previous() and next() on list w/one.......");
    result = true;
    if( le1.add() != efl::LL<efl::Event>::OK )
        result = false;
    ;
    if( (efl::LL<efl::Event>::begin() != &le1 || le1.size() != 1 ))
        result = false;
    if( efl::LL<efl::Event>::begin()->next() != efl::LL<efl::Event>::end())
        result = false;
    if( le1.next() != efl::LL<efl::Event>::end())
        result = false;
    if( efl::LL<efl::Event>::end()->previous() != &le1)
        result = false;

    if(result)
    {
        coln("OK");
    }
    else
    {
        coln("FAILED");
    }

    co( "Check begin(), end(), previous() and next() on list w/three.....");
    if( le2.add() == efl::LL<efl::Event>::OK && le3.add() == efl::LL<efl::Event>::OK)
        result = true;
    if( (efl::LL<efl::Event>::begin() != &le1 || le1.size() != 3 ))
        result = false;
    if( efl::LL<efl::Event>::begin()->next() != &le2)
        result = false;
    if( efl::LL<efl::Event>::begin()->next()->next() != &le3)
        result = false;
    if( efl::LL<efl::Event>::begin()->next()->next()->next() != le3.end())
        result = false;
    if( efl::LL<efl::Event>::end()->previous() != &le3)
        result = false;
    if( efl::LL<efl::Event>::end()->previous()->previous() != &le2)
        result = false;
    if( efl::LL<efl::Event>::end()->previous()->previous()->previous() != &le1)
        result = false;
    if( efl::LL<efl::Event>::end()->previous()->previous()->previous() != le1.begin())
        result = false;
    if( efl::LL<efl::Event>::end()->previous()->previous()->previous()->previous() != le1.end())
        result = false;
    if(result)
    {
        coln("OK");
    }
    else
    {
        coln("FAILED");
    }
#endif //defined TEST_EVENT

#if defined TEST_TIMER
    // now to fool around with Timers
    coln( "\nLL<Timer> tests" );


    // three timers with 1,2 and three counnts remaining. In 5 ticks,
    // first should run 5 times, second twice and 3rd once
    MyTimer   t1(1,1,1);
    MyTimer   t2(2,3,2);
    MyTimer   t3(3,5,3);
    MyOneShotTimer      tos1(1,1);
    efl::LL<efl::Timer> lt1(&t1);
    efl::LL<efl::Timer> lt2(&t2);
    efl::LL<efl::Timer> lt3(&t3);
    efl::LL<efl::Timer> ltos1(&tos1);

    efl::LL<efl::Timer>*  pScratchT;

    co( "efl::LL<efl::Timer>::add()................................................");
    if ( lt1.add() == efl::LL<efl::Timer>::OK && efl::LL<efl::Timer>::size() == 1 &&
            efl::LL<efl::Timer>::begin() == &lt1)
    {
        coln( "OK" );
    }
    else
    {
        coln( "FAILED" );
    }

    co( "efl::LL<efl::Timer>::add().and.add()......................................");
    if ( lt2.add() == efl::LL<efl::Timer>::OK && efl::LL<efl::Timer>::size() == 2 && efl::LL<efl::Timer>::begin() == &lt1 &&
            lt3.add() == efl::LL<efl::Timer>::OK && efl::LL<efl::Timer>::size() == 3 &&
            (pScratchT = efl::LL<efl::Timer>::begin()) == &lt1 &&
            (pScratchT = pScratchT->next()) == &lt2 &&
            (pScratchT = pScratchT->next()) == &lt3 &&
            (pScratchT = pScratchT->next()) == pScratchT->end())
    {
        coln( "OK" );
    }
    else
    {
        coln( "FAILED " );
    }

    co( X("efl::LL<efl::Timer>::doItems(5x1)........................................."));
    delay(1);
    efl::LL<efl::Timer>::doItems();
    delay(1);
    efl::LL<efl::Timer>::doItems();
    delay(1);
    efl::LL<efl::Timer>::doItems();
    delay(1);
    efl::LL<efl::Timer>::doItems();
    delay(1);
    efl::LL<efl::Timer>::doItems();
    coln(millis());
    co( t1.getCallCount());
    co( " ");
    co( t2.getCallCount());
    co( " ");
    co( t3.getCallCount());
    coln(" vs 5 2 1");
    if( t1.getCallCount() == 5 && t2.getCallCount() == 2 && t3.getCallCount() == 1 )
    {
        coln( "OK" );
    }
    else
    {
        coln( "FAILED " );
    }

    t1.setCounter(t1.getPeriod());
    t1.clearCallCount();
    t2.setCounter(t2.getPeriod());
    t2.clearCallCount();
    t3.setCounter(t3.getPeriod());
    t3.clearCallCount();
    co( "efl::LL<efl::Timer>::doItems(3x2).........................................");
    delay(2);
    efl::LL<efl::Timer>::doItems();
    delay(2);
    efl::LL<efl::Timer>::doItems();
    delay(2);
    efl::LL<efl::Timer>::doItems();

    if( t1.getCallCount() == 3 && t2.getCallCount() == 3 && t3.getCallCount() == 2 )
    {
        coln( "OK" );
    }
    else
    {
        coln( "FAILED " );
    }

    //verbose=false;
    //coln("..................................Verbose off");
    //delay(1000);

    co( "efl::LL<efl::Timer>::doItems(3x3).........................................");
    t1.setCounter(t1.getPeriod());
    t1.clearCallCount();
    t2.setCounter(t2.getPeriod());
    t2.clearCallCount();
    t3.setCounter(t3.getPeriod());
    t3.clearCallCount();
    delay(3);
    efl::LL<efl::Timer>::doItems();
    delay(3);
    efl::LL<efl::Timer>::doItems();
    delay(3);
    efl::LL<efl::Timer>::doItems();

    if( t1.getCallCount() == 3 && t2.getCallCount() == 3 && t3.getCallCount() == 3 )
    {
        coln( "OK" );
    }
    else
    {
        coln( "FAILED " );
    }

    t1.setCounter(t1.getPeriod());
    t1.clearCallCount();
    t2.setCounter(t2.getPeriod());
    t2.clearCallCount();
    t3.setCounter(t3.getPeriod());
    t3.clearCallCount();

    co( "efl::LL<efl::Timer>::erase()(x3)..........................................");
    if( lt1.begin()->erase()->erase()->erase() == lt1.end() && lt1.size() == 0 )
    {
        coln( "OK" );
    }
    else
    {
        coln( "FAILED" );
    }

    co( "efl::LL<efl::Timer>::OneShotTimer.........................................");
    efl::LL<efl::Timer>::rs addrs = ltos1.add();
    delay(1);
    efl::LL<efl::Timer>::doItems();
    delay(1);
    efl::LL<efl::Timer>::doItems();
    delay(1);
    efl::LL<efl::Timer>::doItems();
    delay(1);
    efl::LL<efl::Timer>::doItems();
    if (verbose) {
        co( "rs " );
        co(addrs);
        co(" ");
        co(lt1.size());
        co(" ");
        coln(tos1.getCallCount());
    }

    if(addrs == efl::LL<efl::Timer>::OK &&  tos1.clearCallCount() == 1 && lt1.size() == 0 )
    {
        coln( "OK" );
    }
    else
    {
        coln( "FAILED" );
    }

    co( "efl::LL<efl::Timer>::OneShotTimer (same as previous)......................");
    addrs = ltos1.add();
    addMillis(1);
    efl::LL<efl::Timer>::doItems();
    addMillis(1);
    efl::LL<efl::Timer>::doItems();
    //cout << "rs " << addrs << " " << lt1.size() ;
    if(addrs == efl::LL<efl::Timer>::OK &&  tos1.clearCallCount() == 1 && lt1.size() == 0 )
    {
        coln( "OK" );
    }
    else
    {
        coln( "FAILED" );
    }


    co( "efl::LL<efl::Timer>::OneShotTimer/repeat..................................");
    addrs = ltos1.add();
    tos1.setRepeat(true, 1);
    addMillis(1);
    efl::LL<efl::Timer>::doItems();
    addMillis(1);
    efl::LL<efl::Timer>::doItems();
    if(addrs == efl::LL<efl::Timer>::OK &&  tos1.clearCallCount() == 2 && lt1.size() == 1 )
    {
        coln( "OK" );
    }
    else
    {
        coln( "FAILED" );
    }
    coln( "\nLL<efl::Digital> tests" );

#endif //defined TEST_TIMER

#if defined TEST_DIGITAL
    verbose=true;
    //MyDigital(int i, DigitalBit b, int d=1, Polarity p = ACT_HI, efl::uchar interest = (INACTIVE|ACTIVE))

    MyDigital d1 = MyDigital(0, MyDigital::BIT_5, 1, MyDigital::ACT_HI, (MyDigital::INACTIVE|MyDigital::ACTIVE));
    MyDigital d2 = MyDigital(1, MyDigital::BIT_7, 2, MyDigital::ACT_LO, (MyDigital::INACTIVE|MyDigital::ACTIVE));
    MyDigital d3 = MyDigital(1, MyDigital::AN_0, 2, MyDigital::ACT_HI, (MyDigital::INACTIVE|MyDigital::ACTIVE));
    efl::LL<efl::Digital> ld1(&d1); ld1.add();
    //efl::LL<efl::Digital> ld2(&d2); ld2.add();
    //efl::LL<efl::Digital> ld3(&d3); ld3.add();

    efl::LL<efl::Digital>::doItems();
    efl::LL<efl::Digital>::doItems();
    digitalWrite(4, 1);
    efl::LL<efl::Digital>::doItems();
    efl::LL<efl::Digital>::doItems();

#endif //defined TEST_DIGITAL



}

