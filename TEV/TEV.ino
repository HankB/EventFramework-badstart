#if(DEBUG) // debug on Linux
    using namespace std;
    #include <iostream>
    #include <unistd.h>
            // co => console output
    #define co(x) cout << (x) 
    #define coln(x) {cout << (x) << endl;}

    static const int sleepTime=0;

// g++ -Wall -DDEBUG  -o main main.cpp

// now stub out some Arduino calls

unsigned long millisVal=0;
void addMillis(long m) { millisVal += m; }
unsigned long millis(){ return millisVal; }
void delay(unsigned int n) { millisVal+=n;}

#else // run on Arduino

    #include "Arduino.h"

    #define co(x) Serial.print(x);
    #define coln(x) Serial.println(x);
    void sleep(int x){ delay(x*1000);};
    void addMillis(long m) { delay(m);}
    static const int sleepTime=1;
#endif

// #define NDEBUG  uncomment to disable asserts.
#include <assert.h>
//#include <algorithm>    // std::min

//namespace efl { // event framework library

static bool verbose=false;      // turn on/off verbose crap. Make it const and the compiler is free to optimize away the code

/**
 * LL is a template class which adds the linked list to objects of type Item
 * which are the various Event types
 */

template<class Item> class LL {
private:
	LL* 	    pNext;          // point to next item in list
    static LL<Item>& sentinel(){static LL<Item> rc=LL<Item>((Item*)0); return rc;};
	Item*    pItem;                 // event descriptor
	LL():pItem((Item*)0),pNext(this){}; // can't construct w/out an Item except for the special case in sentinel()
public:
 	enum rs {
		OK=0,                   // return status - operation succeeded
		NAK=1,                  // general bad status
		BAD_DUP=2               // duplicate e.g already in a list.
	};

	LL(Item*pI):pNext(this),pItem(pI){};    // construct from an Item
	rs add();                               // add an Item to the tail of the queue
	rs push();                              // push an Item on to he front of the queue
	LL* next(){return pNext;};              // return pointer to next iutem in list
	LL* previous();                         // pointer to previous item in list
	static LL* begin(){return sentinel().pNext;};   // pointer to first item in list (past tthe sentinel)
	static LL* end(){return &sentinel();};  // pointer to end of list (e.g. the sentinel)
	LL* erase();                            // remove this item from the list
    static int& size(){static int elementCount=0; return elementCount;};
    static void doItems();      // each specialization expected to provide their own 'doItems()'

    static void walk();
};

template<class Item>
typename LL<Item>::rs LL<Item>::add() {
    if( this == end())            // this would be bad!
        return BAD_DUP;
	if (pNext == this) {                // should be point to itself right now
		LL<Item>* pLL = LL<Item>::sentinel().pNext; // point to head of list

		while (pLL->pNext != &LL<Item>::sentinel()) // Does this one point toward the sentinel (i.e. end of list)
			pLL = pLL->pNext;

		pLL->pNext = this;              // make end of list point to the one to add
        pNext = &LL<Item>::sentinel();     // and now mark it as the new end of the list
        size()++;
		return LL<Item>::OK;
	} else {
		return LL<Item>::BAD_DUP;
	}
}

template<class Item>
typename LL<Item>::rs LL<Item>::push()
{
    if( this != pNext ) // already in the list"
        return BAD_DUP;
    pNext = sentinel().pNext;
    sentinel().pNext = this;
    size()++;
    return OK;
}


template<class Item>
LL<Item>* LL<Item>::previous()
{
    LL<Item>* pLL = begin();
    while(pLL != end())
        if(pLL->next() == this)
            return pLL;
        else
            pLL = pLL->next();
    return pLL;
}

#if(DEBUG)
template<class Item>
void LL<Item>::walk()
{
    coln(""); co("walk("); co(size()); co(") ");
    LL<Item>*  pLL = LL<Item>::sentinel().pNext;
    co( (void*)&sentinel() ); co( " -> ");
    while( pLL != &LL<Item>::sentinel() )
    {
        co( pLL); co( " -> ");
        pLL = pLL->pNext;
        //sleep(sleepTime);
    }
    coln( pLL );
}
#endif

template<class Item>
LL<Item>* LL<Item>::erase()
{
    if( this == pNext )                           // marked as not in list?
        return begin();                           // does anything else make sense here? This should be safe for the caller.
    LL<Item>*  pLL = &LL<Item>::sentinel();       // start at the head end of the list
    while( pLL->pNext != &LL<Item>::sentinel() )  // and iterate until we've gone through the list (should never happen!
    {
        if( this == pLL->pNext )
        {
            pLL->pNext = pNext;
            pNext = this;
            size()--;
            return pLL->pNext;
        }
        pLL = pLL->pNext;
    }
    assert(0);
    return pLL->pNext;      // should never get here
}

class Event { // callback makes this a one shot event (won't execute until it is added back to the list.)
public:
    Event(){};
    virtual bool callback(){ if (verbose) coln( "Event:"); return false;};
};

class MyEvent : public Event {
protected:
    int     id;
public:
    MyEvent(int n):id(n){};
    virtual bool callback(){ if (verbose) {co( "MyEvent:"); coln( id);} return false;};
};

class RepeatEvent: public MyEvent { // this event remains in the queue and will execute each time Event list is examined
public:
    RepeatEvent(int n):MyEvent(n){};
    virtual bool callback(){ if (verbose) {co( "RepeatEvent:"); coln( id);} return true; };
};

template<>
void LL<Event>::doItems()
{
    for(LL<Event>* pLL = begin(); pLL != end(); )
        if(!pLL->pItem->callback())
            pLL = pLL->erase();         // remove from list
}

typedef unsigned long ulong; // unsigned long gets a bit tedious

class Timer { // default timer is as oneshot ( data member is redundant but kept here to keep this object more general)
private:
    ulong   counter;
    ulong   period;
public:
    Timer(unsigned long c=1, unsigned long p=0):counter(c),period(p){}; // default to fire once after 1 ms
    virtual bool callback(ulong late){ if (verbose) {coln( "Timer:");} return false;};
    ulong getCounter(){ return counter; }
    void setCounter(ulong c){ counter=c; }
    ulong getPeriod(){ return period; }
    void setPeriod(ulong p){ period=p; }

};

template<>
void LL<Timer>::doItems()
{
    static ulong prevMillis = 0;    // start when execution starts 
    ulong   nowMillis = millis();   // time now
    ulong   deltaMillis = nowMillis - prevMillis;

    if(!deltaMillis)
        return;

    //if(verbose) walk();

    // iterate through timers to see which ones have downconted to or beyond zero
    for(LL<Timer>* pLL = begin(); pLL != end(); )
    {
        ulong late=deltaMillis - pLL->pItem->getCounter();
        if( pLL->pItem->getCounter() <= deltaMillis ) 
        {
            if( pLL->pItem->callback(late) && pLL->pItem->getPeriod() > 0 ) // need both period and 'true' response to keep active
            {
                // policy decision here. Do we set the counter to 0 or less if
                // we're late by the period or more? No, I guess...
                if( late >= pLL->pItem->getPeriod())
                    pLL->pItem->setCounter(1);
                else
                    pLL->pItem->setCounter(pLL->pItem->getPeriod()-late);
                pLL = pLL->next();
            }
            else
            {
                pLL = pLL->erase();
            }
        }
        else
        {
            pLL->pItem->setCounter(pLL->pItem->getCounter()-deltaMillis);
            pLL = pLL->next();
        }
    //sleep(1);
    }
    prevMillis = nowMillis;
}

class MyTimer: public Timer  // periodic timer by default
{
private:
    virtual bool callback(ulong late){ callCount++; if (verbose) {co( "MyTimer: "); coln(id);} return true;};
    int     id;
    int     callCount;
public:
    MyTimer(int i, ulong c=1, ulong p=1):Timer(c,p),id(i),callCount(0){};
    int getCallCount(){ return callCount;};
    int clearCallCount(){ int rc=callCount; callCount=0; return rc;};
};

class MyOneShotTimer: public Timer 
{
private:
    virtual bool callback(ulong late)
    {
        callCount++;
        if (verbose) {co( "MyOneShotTimer: "); coln(id);}
        return retVal;
    };
    int     id;
    int     callCount;
    bool    retVal;
public:
    MyOneShotTimer(int i, unsigned long c):Timer(c,0),id(i),callCount(0),retVal(false){};
    int getCallCount(){ return callCount;};
    int clearCallCount(){ int rc=callCount; callCount=0; return rc;};
    void setRepeat(bool r, ulong p) { retVal=r; setPeriod(p); } // enforce consistency in args???
};



/*
class Ditital { // Monitor a digital input
private:
    ulong   counter;
    ulong   period;
public:
    Timer(unsigned long c=1, unsigned long p=0):counter(c),period(p){}; // default to fire once after 1 ms
    virtual bool callback(ulong late){ if (verbose) {coln( "Timer:");} return false;};
    ulong getCounter(){ return counter; }
    void setCounter(ulong c){ counter=c; }
    ulong getPeriod(){ return period; }
    void setPeriod(ulong p){ period=p; }

};

template<>
void LL<Timer>::doItems()
{
    static ulong prevMillis = 0;    // start when execution starts 
    ulong   nowMillis = millis();   // time now
    ulong   deltaMillis = nowMillis - prevMillis;

    if(!deltaMillis)
        return;

    if(verbose) walk();

    // iterate through timers to see which ones have downconted to or beyond zero
    for(LL<Timer>* pLL = begin(); pLL != end(); )
    {
        ulong late=deltaMillis - pLL->pItem->getCounter();
        if( pLL->pItem->getCounter() <= deltaMillis ) 
        {
            if( pLL->pItem->callback(late) && pLL->pItem->getPeriod() > 0 ) // need both period and 'true' response to keep active
            {
                // policy decision here. Do we set the counter to 0 or less if
                // we're late by the period or more? No, I guess...
                if( late >= pLL->pItem->getPeriod())
                    pLL->pItem->setCounter(1);
                else
                    pLL->pItem->setCounter(pLL->pItem->getPeriod()-late);
                pLL = pLL->next();
            }
            else
            {
                pLL = pLL->erase();
            }
        }
        else
        {
            pLL->pItem->setCounter(pLL->pItem->getCounter()-deltaMillis);
            pLL = pLL->next();
        }
    //sleep(1);
    }
    prevMillis = nowMillis;
}

*/


#if(DEBUG) // debug on Linux
int main()
#else
void setup()
{
    Serial.begin(115200);
}

void loop()
#endif
{
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
        {coln( "OK" );}
    else
        {coln("FAILED "); /* co( le1.erase() ); co( " " ); co(  LL<Event>::begin() ); co( " " ); coln(le1.size());*/}

    co( "add() ..........................................................");
    if( le1.add() ==  LL<Event>::OK && (pScratchE = LL<Event>::begin()) == &le1 && le1.size() == 1
        && pScratchE->next() == LL<Event>::end() )
        {coln( "OK" );}
    else
        {coln( "FAILED " );}

    co( "erase().........................................................");
    if( le1.erase() == LL<Event>::end() && le1.size() == 0 )
        {coln( "OK" );}
    else
        {coln( "FAILED " );}

    le1.add();
    co( "add() /dup......................................................");
    if( le1.add() == LL<Event>::BAD_DUP && LL<Event>::size() == 1  )
        {coln( "OK" );}
    else
        {coln( "FAILED " );}

    le2.push();

    co( "erase()/first...................................................");
    if( (pScratchE=le2.erase()) == &le1 && le1.begin() == &le1 && le1.size() == 1 )
        {coln("OK");}
    else
        {coln("FAILED");}

    le2.push();
    co( "erase()/second..................................................");
    if( (pScratchE=le1.erase()) == le1.end() && le1.begin() == &le2 && le1.size() == 1 )
        {coln("OK");}
    else
        {coln("FAILED");}

    co( "Adding 1,3 and removing the first...............................");
    if( le1.add() == LL<Event>::OK && le3.add() == LL<Event>::OK  && le2.erase() == le2.begin() && le2.size() == 2 )
        {coln("OK");}
    else
        {coln("FAILED");}

    co( "Adding 2 to front and removing middle...........................");
    if( le2.add() == LL<Event>::OK && le3.erase() == le2.begin()->next() && le2.size() == 2 )
        {coln("OK");}
    else
        {coln("FAILED");}

    co( "Removing last and then only remaining Item......................");
    if( le1.erase() == &le2 && le2.erase() == le2.end() && le2.size() == 0 )
        {coln("OK");}
    else
        {coln("FAILED");}

    co( "Add and remove and try to remove sentinel.......................");
    if( le1.add() ==  LL<Event>::OK && le1.erase() == le1.end() && le2.size() == 0
        && le1.end()->erase() == le1.end() && le2.size() == 0 )
        {coln("OK");}
    else
        {coln("FAILED");}

    co( "Try to add sentinel.............................................");
    if( le1.end()->add() == LL<Event>::BAD_DUP && le2.size() == 0 )
        {coln("OK");}
    else
        {coln("FAILED");}

    co( "Try to remove sentinel..........................................");
    if( le1.end()->erase() ==  LL<Event>::end() && le2.size() == 0 )
        {coln("OK");}
    else
        {coln("FAILED");}

    
    co( "Check begin(), end(), previous() and next() on empty list.......");
    bool result=true;   // set default to true
    if( !(LL<Event>::end() ==  LL<Event>::begin() && le2.size() == 0 ))
        result = false;
    if( LL<Event>::begin()->next() != LL<Event>::end())
        result = false;
    if( LL<Event>::begin()->previous() != LL<Event>::end())
        result = false;

   if(result) 
        {coln("OK");}
    else
        {coln("FAILED");}

    co( "Check begin(), end(), previous() and next() on list w/one.......");
    result = true;
    if( le1.add() != LL<Event>::OK )
        result = false;;
    if( (LL<Event>::begin() != &le1 || le1.size() != 1 ))
        result = false;
    if( LL<Event>::begin()->next() != LL<Event>::end())
        result = false;
    if( le1.next() != LL<Event>::end())
        result = false;
    if( LL<Event>::end()->previous() != &le1)
        result = false;

    if(result) 
        {coln("OK");}
    else
        {coln("FAILED");}
    
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
        {coln("OK");}
    else
        {coln("FAILED");}
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
        { coln( "OK" ); }
    else
        { coln( "FAILED" ); }

    co( "LL<Timer>::add().and.add()......................................");
    if ( lt2.add() == LL<Timer>::OK && LL<Timer>::size() == 2 && LL<Timer>::begin() == &lt1 &&
            lt3.add() == LL<Timer>::OK && LL<Timer>::size() == 3 && 
            (pScratchT = LL<Timer>::begin()) == &lt1 &&
            (pScratchT = pScratchT->next()) == &lt2 &&
            (pScratchT = pScratchT->next()) == &lt3 &&
            (pScratchT = pScratchT->next()) == pScratchT->end()) 
        { coln( "OK" ); }
    else
        { coln( "FAILED " ); }
        
    delay(1000);   
    verbose=true;
    coln("Verbose on");
    delay(1000);
    
    coln( "LL<Timer>::doItems(5x1).........................................");
    delay(1); LL<Timer>::doItems(); coln(millis());
    delay(1); LL<Timer>::doItems(); coln(millis());
    delay(1); LL<Timer>::doItems(); coln(millis());
    delay(1); LL<Timer>::doItems(); coln(millis());
    delay(1); LL<Timer>::doItems(); coln(millis());
    co( t1.getCallCount()); co( " "); 
    co( t2.getCallCount()); co( " "); 
    coln( t3.getCallCount()); 
    if( t1.getCallCount() == 5 && t2.getCallCount() == 2 && t3.getCallCount() == 1 )
        { coln( "OK" ); }
    else
        { coln( "FAILED " ); }
        

    //millisVal++; LL<Timer>::doItems(); 
    //millisVal++; LL<Timer>::doItems(); 
    //millisVal++; LL<Timer>::doItems(); 
    t1.setCounter(t1.getPeriod());  t1.clearCallCount();
    t2.setCounter(t2.getPeriod());  t2.clearCallCount();
    t3.setCounter(t3.getPeriod());  t3.clearCallCount();
    co( "LL<Timer>::doItems(3x2).........................................");
    delay(2); LL<Timer>::doItems(); 
    delay(2); LL<Timer>::doItems(); 
    delay(2); LL<Timer>::doItems(); 

    if( t1.getCallCount() == 3 && t2.getCallCount() == 3 && t3.getCallCount() == 2 )
        { coln( "OK" ); }
    else
        { coln( "FAILED " ); }

    verbose=false;
    coln("..................................Verbose off");
    delay(1000);

    co( "LL<Timer>::doItems(3x3).........................................");
    t1.setCounter(t1.getPeriod());  t1.clearCallCount();
    t2.setCounter(t2.getPeriod());  t2.clearCallCount();
    t3.setCounter(t3.getPeriod());  t3.clearCallCount();
    delay(3); LL<Timer>::doItems(); 
    delay(3); LL<Timer>::doItems(); 
    delay(3); LL<Timer>::doItems(); 

    if( t1.getCallCount() == 3 && t2.getCallCount() == 3 && t3.getCallCount() == 3 )
        { coln( "OK" ); }
    else
        { coln( "FAILED " ); }

    t1.setCounter(t1.getPeriod()); t1.clearCallCount(); 
    t2.setCounter(t2.getPeriod()); t2.clearCallCount(); 
    t3.setCounter(t3.getPeriod()); t3.clearCallCount(); 
    
    co( "LL<Timer>::erase()(x3)..........................................");
    if( lt1.begin()->erase()->erase()->erase() == lt1.end() && lt1.size() == 0 )
        { coln( "OK" ); }
    else
        { coln( "FAILED" ); }

    co( "LL<Timer>::OneShotTimer.........................................");
    LL<Timer>::rs addrs = ltos1.add();
    delay(1);LL<Timer>::doItems();delay(1);LL<Timer>::doItems();
    delay(1);LL<Timer>::doItems();delay(1);LL<Timer>::doItems();
    if (verbose) { co( "rs " ); co(addrs); co(" "); co(lt1.size()); co(" "); coln(tos1.getCallCount());}

    if(addrs == LL<Timer>::OK &&  tos1.clearCallCount() == 1 && lt1.size() == 0 )
        { coln( "OK" ); }
    else
        { coln( "FAILED" ); }

    co( "LL<Timer>::OneShotTimer (same as previous)......................");
    addrs = ltos1.add();
    addMillis(1);LL<Timer>::doItems();addMillis(1);LL<Timer>::doItems();
    //cout << "rs " << addrs << " " << lt1.size() ;
    if(addrs == LL<Timer>::OK &&  tos1.clearCallCount() == 1 && lt1.size() == 0 )
        { coln( "OK" ); }
    else
        { coln( "FAILED" ); }


    co( "LL<Timer>::OneShotTimer/repeat..................................");
    addrs = ltos1.add();
    tos1.setRepeat(true, 1);
    addMillis(1);LL<Timer>::doItems();addMillis(1);LL<Timer>::doItems();
    if(addrs == LL<Timer>::OK &&  tos1.clearCallCount() == 2 && lt1.size() == 1 )
        { coln( "OK" ); }
    else
        { coln( "FAILED" ); } 

#if(DEBUG) // debug on Linux
    return 0;
#else
    delay(180000); // 3 minutes
#endif
}

