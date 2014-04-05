#define co(x) Serial.print(x)
#define coln(x) Serial.println(x)
#include <MemoryFree.h>

// lets not conflict with the global namespace

enum rs {   OK=0,                   // return status - operation succeeded
            NAK=1,                  // general bad status
            BAD_DUP                 // duplicate e.g already in a list.
            };

class Event;

class Event {
public:

    rs add();                       // add an event to the RTR list
    rs remove();                    // remove this event from the list
    static int doEvents();          // iterate through events
protected:
    Event():pRTRnext(this){ };
    virtual void callBack(){};    // callback when Event fires
private:
    static Event    s;              // sentinel, holds head of list abd poited to byt tail of list
                                    // so we can avoid null pointers. Points to itself when list is empty
    Event*          pRTRnext;       // point to next in list (or 0 when last in list)
};

Event Event::s;                     // sentinel, head/tail of list so we can avoid null pointers

rs Event::add()
{
    Event* pList = &s;              // point to the list

    if( this != pRTRnext )          // already in queue?
        return BAD_DUP;
    // find the end of the list
    while( pList->pRTRnext != &s)    // whilel not the end of the list
        pList = pList->pRTRnext;    // next link in the list
    pList->pRTRnext = this;         // tak onto the end of the list
    pRTRnext = &s;

    return OK;
}

rs Event::remove()           // remove this event from the list
{
    Event* pList = &s;              // point to the list

    if( this == pRTRnext )          // not in queue?
        return NAK;

    // find this element in the list
    while( pList->pRTRnext != &s)    // while not the end of the list
    {
        if( pList->pRTRnext == this ) // found it!
        {
            pList->pRTRnext = pRTRnext; // remove 'this' from list
            pRTRnext = this;        // mark as no longer in list
            return OK;
        }
        pList = pList->pRTRnext;    // next link in the list
    }
    
    // 'this' marked as in list and not found - we are screwed!
    return NAK;
}

int Event::doEvents()
{
    Event* pE = s.pRTRnext;


    while( (pE = s.pRTRnext) != &s )// loop while there entries in the RTR queue
    {
        while( pE != &s )           // iterate through the RTR queue
        {
            Event* pCurrent(pE);    // save a pointer to current so we can remove from list before executing
            pE = pE->pRTRnext;      // step to next
            pCurrent->remove();     // remove from the list
            pCurrent->callBack();   // execute the callback
        }
    }
    return 0;
}

class MyEvent:public Event {
public:
    void callBack();
    MyEvent(int i):Event(),id(i){};
private:
    int     id;
};

void MyEvent::callBack()
{
    Serial.print( "MyEvent:"); Serial.println(id);
}


class Timer : public Event {
public:
    Timer( int t=1000, int r=0):Event(), et(t), resetVal(r), pTnext(this){};
    static int doTimers(int msec);
    rs removeT(Timer* pTprev);
    rs add();
private:
    long            et;             // count down timer
    long            resetVal;       // value to reset to when complete
    static Timer    s;              // sentinel, head/tail of timer list so we can avoid null pointers
    Timer*          pTnext;         // point to next timer in list (or 0 when last in list)
                                    // or self when not in a list
};

rs Timer::removeT(Timer* pTprev)
{
    if(this == pTnext)              // not linked into the list
    {
        return NAK;
    }
    else
    {
        pTprev->pTnext = pTnext;
        pTnext = this;              // mark as not in list
        return OK;
    }
}

rs Timer::add()
{
    if( this == pTnext ) // not currently in the timer list?
    {
        pTnext = s.pTnext;          // point to what was first entry
        s.pTnext = this;            // sentinel now points to this
        return OK;
    }
    else
    {
        return BAD_DUP;             // already in the queue
    }
}

Timer Timer::s;                     // sentinel, head/tail of timer list so we can avoid null pointers

int Timer::doTimers(int msec)
{
    Timer* pT = s.pTnext;           // point to the sentinel
    Timer* pTprev = &s;             // save pointer to previous

    while( pT != &s )               // iterate through the Timer queue
    {
        if( --(pT->et) <= 0 )       // timer expired?
        {
            if( pT->resetVal != 0)  // periodic timer?
            {
                Timer* pTmp = pT;   // save pointer so we can execute callback after list manuipulation
                pTprev = pTmp;
                pT->et = pT->resetVal-msec+1;
                pT = pT->pTnext;       // Next!
                pTmp->callBack();   // execute the callback
            }
            else // one shot, remove from timer queue
            {
                Timer* pTmp = pT; // save pointer so we can execute callback after list manuipulation
                pT->removeT(pTprev);
                // std::cout << "unlinked " << pTprev << " " << pT << " " << pT->pTnext << std::endl;
                // Timer::walk();
                pT = pTprev->pTnext;       // Next!
                pTmp->callBack();   // execute the callback
            }
        }
        else
        {
            pTprev = pT;
            pT = pT->pTnext;       // Next!
        }
    }
    return 0;
}


    // App specific

static const int heartbeat = 13;

class MyTimer : public Timer {
protected:
    virtual void callBack();// callback when Event fires
    int     id;
public:
    MyTimer( int i, int delay=1, int interval=0):Timer(delay,interval),id(i){};
};

void MyTimer::callBack()
{
    digitalWrite( heartbeat, id);
}

class MemTimer : public Timer {
public:
    MemTimer(int t=1000, int r=0):Timer(t,r){};
    MemTimer(const MemTimer& e){coln("MyTimer copied");};
    void callBack(int late) {        // callback when Timer fires
        Serial.print("freeMemory()=");
        Serial.println(freeMemory());
    };
};




MyTimer     t1( HIGH, 1000, 200 );
MyTimer     t2( LOW,  1020, 200 );

MemTimer    tMem(2000, 2000); // initial delay is 2s and repeat every 2 s


void setup()
{
    Serial.begin( 115200 );
    pinMode( heartbeat, OUTPUT );
    t1.add();
    t2.add();
    tMem.add();
}

void loop()
{
    static unsigned long prevMillis = 0;
    static unsigned long prevMillisX = 0;

    unsigned long currentMillis = millis();

    if( currentMillis != prevMillis)
    {
        Timer::doTimers(1); 
        prevMillis = currentMillis;
    }
    
    if( currentMillis -  prevMillisX >=2000)
    {
        Serial.print("freeMemory()=");
        Serial.println(freeMemory());
        prevMillisX = currentMillis;
    }
}

