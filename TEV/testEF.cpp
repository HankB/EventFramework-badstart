#if defined AVR // run on Arduino

#include "Arduino.h"

//typedef unsigned long ulong; // unsigned long int gets a bit tedious
typedef unsigned long int uLong;


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

typedef unsigned long uLong; // unsigned long int gets a bit tedious

using namespace std;
#include <iostream>
#include <unistd.h>
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

#endif // defined AVR 

// #define NDEBUG  uncomment to disable asserts.
#include <assert.h>
//#include <algorithm>    // std::min

static bool verbose=false;      // turn on/off verbose crap. Make it const and the compiler is free to optimize away the code

#include "EventFramework.h"


class MyTimer: 
public Timer  // periodic timer by default
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
  Timer(c,p),id(i),callCount(0) {
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
public Timer
{
private:
  virtual bool callback(uLong late)
  {
    callCount++;
    if (verbose) {
      co( "MyOneShotTimer: ");
      coln(id);
    }
    return retVal;
  };
  int     id;
  int     callCount;
  bool    retVal;
public:
  MyOneShotTimer(int i, unsigned long c):
  Timer(c,0),id(i),callCount(0),retVal(false) {
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


// derive my event from the generic one so I can provide my own callback.
class MyEvent : 
public Event {
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
public MyEvent { // this event remains in the queue and will execute each time Event list is examined
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
  MyEvent         e1(1);
  MyEvent         e2(2);
  MyEvent         e3(3);
  RepeatEvent     re1(4);

  LL<Event>       le1(&e1);
  LL<Event>       le2(&e2);
  LL<Event>       le3(&e3);
  LL<Event>       lre1(&re1);

  LL<Event>*  pScratchE;


  /* codify the regression tests */

  coln( "LL<Event> tests");

#if defined(NOWHERE)

  delay(10000);
  co( "erase() w/o add() ..............................................");
  if( le1.erase() == LL<Event>::begin() && le1.size() == 0 )
  {
    coln( "OK" );
  }
  else
  {
    coln("FAILED "); /* co( le1.erase() ); co( " " ); co(  LL<Event>::begin() ); co( " " ); coln(le1.size());*/
  }

  co( "add() ..........................................................");
  if( le1.add() ==  LL<Event>::OK && (pScratchE = LL<Event>::begin()) == &le1 && le1.size() == 1
    && pScratchE->next() == LL<Event>::end() )
  {
    coln( "OK" );
  }
  else
  {
    coln( "FAILED " );
  }

  co( "erase().........................................................");
  if( le1.erase() == LL<Event>::end() && le1.size() == 0 )
  {
    coln( "OK" );
  }
  else
  {
    coln( "FAILED " );
  }

  le1.add();
  co( "add() /dup......................................................");
  if( le1.add() == LL<Event>::BAD_DUP && LL<Event>::size() == 1  )
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
  if( le1.add() == LL<Event>::OK && le3.add() == LL<Event>::OK  && le2.erase() == le2.begin() && le2.size() == 2 )
  {
    coln("OK");
  }
  else
  {
    coln("FAILED");
  }

  co( "Adding 2 to front and removing middle...........................");
  if( le2.add() == LL<Event>::OK && le3.erase() == le2.begin()->next() && le2.size() == 2 )
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
  if( le1.add() ==  LL<Event>::OK && le1.erase() == le1.end() && le2.size() == 0
    && le1.end()->erase() == le1.end() && le2.size() == 0 )
  {
    coln("OK");
  }
  else
  {
    coln("FAILED");
  }

  co( "Try to add sentinel.............................................");
  if( le1.end()->add() == LL<Event>::BAD_DUP && le2.size() == 0 )
  {
    coln("OK");
  }
  else
  {
    coln("FAILED");
  }

  co( "Try to remove sentinel..........................................");
  if( le1.end()->erase() ==  LL<Event>::end() && le2.size() == 0 )
  {
    coln("OK");
  }
  else
  {
    coln("FAILED");
  }


  co( "Check begin(), end(), previous() and next() on empty list.......");
  bool result=true;   // set default to true
  if( !(LL<Event>::end() ==  LL<Event>::begin() && le2.size() == 0 ))
    result = false;
  if( LL<Event>::begin()->next() != LL<Event>::end())
    result = false;
  if( LL<Event>::begin()->previous() != LL<Event>::end())
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
  if( le1.add() != LL<Event>::OK )
    result = false;
  ;
  if( (LL<Event>::begin() != &le1 || le1.size() != 1 ))
    result = false;
  if( LL<Event>::begin()->next() != LL<Event>::end())
    result = false;
  if( le1.next() != LL<Event>::end())
    result = false;
  if( LL<Event>::end()->previous() != &le1)
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
  if( le2.add() == LL<Event>::OK && le3.add() == LL<Event>::OK)
    result = true;
  if( (LL<Event>::begin() != &le1 || le1.size() != 3 ))
    result = false;
  if( LL<Event>::begin()->next() != &le2)
    result = false;
  if( LL<Event>::begin()->next()->next() != &le3)
    result = false;
  if( LL<Event>::begin()->next()->next()->next() != le3.end())
    result = false;
  if( LL<Event>::end()->previous() != &le3)
    result = false;
  if( LL<Event>::end()->previous()->previous() != &le2)
    result = false;
  if( LL<Event>::end()->previous()->previous()->previous() != &le1)
    result = false;
  if( LL<Event>::end()->previous()->previous()->previous() != le1.begin())
    result = false;
  if( LL<Event>::end()->previous()->previous()->previous()->previous() != le1.end())
    result = false;
  if(result)
  {
    coln("OK");
  }
  else
  {
    coln("FAILED");
  }
#endif //defined(NOWHERE)

  // now to fool around with Timers
  coln( "\nLL<Timer> tests" );


  // three timers with 1,2 and three counnts remaining. In 5 ticks,
  // first should run 5 times, second twice and 3rd once
  MyTimer   t1(1,1,1);
  MyTimer   t2(2,3,2);
  MyTimer   t3(3,5,3);
  MyOneShotTimer      tos1(1,1);
  LL<Timer> lt1(&t1);
  LL<Timer> lt2(&t2);
  LL<Timer> lt3(&t3);
  LL<Timer> ltos1(&tos1);

  LL<Timer>*  pScratchT;

  co( "LL<Timer>::add()................................................");
  if ( lt1.add() == LL<Timer>::OK && LL<Timer>::size() == 1 &&
    LL<Timer>::begin() == &lt1)
  {
    coln( "OK" );
  }
  else
  {
    coln( "FAILED" );
  }

  co( "LL<Timer>::add().and.add()......................................");
  if ( lt2.add() == LL<Timer>::OK && LL<Timer>::size() == 2 && LL<Timer>::begin() == &lt1 &&
    lt3.add() == LL<Timer>::OK && LL<Timer>::size() == 3 &&
    (pScratchT = LL<Timer>::begin()) == &lt1 &&
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

  co( "LL<Timer>::doItems(5x1).........................................");
  delay(1);
  LL<Timer>::doItems();
  delay(1);
  LL<Timer>::doItems();
  delay(1);
  LL<Timer>::doItems();
  delay(1);
  LL<Timer>::doItems();
  delay(1);
  LL<Timer>::doItems();
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
  co( "LL<Timer>::doItems(3x2).........................................");
  delay(2);
  LL<Timer>::doItems();
  delay(2);
  LL<Timer>::doItems();
  delay(2);
  LL<Timer>::doItems();

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

  co( "LL<Timer>::doItems(3x3).........................................");
  t1.setCounter(t1.getPeriod());
  t1.clearCallCount();
  t2.setCounter(t2.getPeriod());
  t2.clearCallCount();
  t3.setCounter(t3.getPeriod());
  t3.clearCallCount();
  delay(3);
  LL<Timer>::doItems();
  delay(3);
  LL<Timer>::doItems();
  delay(3);
  LL<Timer>::doItems();

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

  co( "LL<Timer>::erase()(x3)..........................................");
  if( lt1.begin()->erase()->erase()->erase() == lt1.end() && lt1.size() == 0 )
  {
    coln( "OK" );
  }
  else
  {
    coln( "FAILED" );
  }

  co( "LL<Timer>::OneShotTimer.........................................");
  LL<Timer>::rs addrs = ltos1.add();
  delay(1);
  LL<Timer>::doItems();
  delay(1);
  LL<Timer>::doItems();
  delay(1);
  LL<Timer>::doItems();
  delay(1);
  LL<Timer>::doItems();
  if (verbose) {
    co( "rs " );
    co(addrs);
    co(" ");
    co(lt1.size());
    co(" ");
    coln(tos1.getCallCount());
  }

  if(addrs == LL<Timer>::OK &&  tos1.clearCallCount() == 1 && lt1.size() == 0 )
  {
    coln( "OK" );
  }
  else
  {
    coln( "FAILED" );
  }

  co( "LL<Timer>::OneShotTimer (same as previous)......................");
  addrs = ltos1.add();
  addMillis(1);
  LL<Timer>::doItems();
  addMillis(1);
  LL<Timer>::doItems();
  //cout << "rs " << addrs << " " << lt1.size() ;
  if(addrs == LL<Timer>::OK &&  tos1.clearCallCount() == 1 && lt1.size() == 0 )
  {
    coln( "OK" );
  }
  else
  {
    coln( "FAILED" );
  }


  co( "LL<Timer>::OneShotTimer/repeat..................................");
  addrs = ltos1.add();
  tos1.setRepeat(true, 1);
  addMillis(1);
  LL<Timer>::doItems();
  addMillis(1);
  LL<Timer>::doItems();
  if(addrs == LL<Timer>::OK &&  tos1.clearCallCount() == 2 && lt1.size() == 1 )
  {
    coln( "OK" );
  }
  else
  {
    coln( "FAILED" );
  }
}

