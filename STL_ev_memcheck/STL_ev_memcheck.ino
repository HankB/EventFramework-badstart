#include <MemoryFree.h>

char dummy;
/**
    * Build a test of the Event system using STL vectors
    Will implement Plain Events and Timers
*/
    #define co(x) Serial.print(x)
    #define coln(x) Serial.println(x)
    
    #include <iterator>
    #include <list>
    #include <pnew.cpp>
    using std::list;

    /**
    * The Event just encapsulates the callback. Application code can add an Event to the queue
    * and it will be executed (and removed from the queue) the next time the queue is processed
    */

class Event {
public:
    static int doEvents();          // iterate through events
    void add() {events.push_back(this);};
    list<Event*>::iterator remove(list<Event*>::iterator i){return events.erase(i);}
    void remove(){ events.remove(this);};

//    virtual ~Event();
protected:
    virtual void callBack(){coln("Event::callBack called!");};        // callback when Event fires
private:
    static list<Event*>     events;        // Event queue

};

list<Event*>     Event::events;        // Event queue

    /**
    *   Iterate through the Event list, remove each event from the list and execute the callback.
    *   The event is removed from the list prior to calling back in case the caller adds the event back to the list
    */

int Event::doEvents()
{
    int count=0;
    list<Event*>::iterator i = events.begin();
    while(i != events.end()) {
        Event* ev = *i; // save a pointer for the event before we remove it
        i = (*i)->remove(i); // returns i == next element following erased item
        ev->callBack();
        count++;
    }
    return count;
}

class MyEvent : public Event {
public:
    int     i;
    MyEvent(int n):i(n){};
    MyEvent(const MyEvent& e):i(e.i){coln("MyEvent copied");};
//    ~MyEvent(){};
// protected:
    void callBack();        // callback when Event fires
};

void MyEvent::callBack(){co("MyEvent callback "); coln(i);}        // callback when Event fires


/**
* Timer encapsulates the callback, count down timer and repeat timer. Every millisecond the Timer list is examined
* and the count down timer decremented by the number of milliseconds since the last examination. When the counter decrements to zero (or less if late)
* the callback is executed (and notified if late.) If the reset value is zero, the Timer is removed form the list (one shot behavior.) This is done prior to the
* callback being issued in case the callback adds the timer back ot the list. If the resetVal is non-zero, the timer is reset to that value.
*/

class Timer {
public:
    Timer( int t=1000, int r=0):et(t), resetVal(r){};
    static int doTimers();
    virtual void callBack(int late){coln("Timer::callBack called!");};// callback when Timer fires
    long    getResetVal() { return resetVal; };
    void add() {timers.push_back(this);};
    list<Timer*>::iterator remove(list<Timer*>::iterator i){return timers.erase(i);}
    void remove(){ timers.remove(this);};
private:
    static unsigned long prevMsec;   // previous millisecond reading
    long        et;         // count down timer
    long        resetVal;   // value to which timer is reset (period)
                            // zero means timer is a one shot
    static list<Timer*>     timers;
};

list<Timer*>     Timer::timers;
unsigned long Timer::prevMsec;   // previous millisecond reading


int Timer::doTimers()
{
    int count;
    unsigned long   currentMillis = millis();
    if( prevMsec != currentMillis) {
        int delta = currentMillis - prevMsec;
        prevMsec = currentMillis;
        list<Timer*>::iterator i = timers.begin();
        while(i != timers.end()) {
            Timer*  tm = *i;    // save a pointer for the event before we (might) remove it (which we do
                                // prior to executing the callback in case the callback adds it back to the list<>
            if(tm->et <= delta) {
                int     late = delta - tm->et - 1;         // number of msec by which the timer is late being called
                if( 0 == tm->resetVal) // one shot timer?
                    i = timers.erase(i); // returns i == next element following erased item
                else
                    tm->et += tm->resetVal;
                tm->callBack(late);
                count++;                    // count timers being processed
            }
            else
            {
                tm->et -= delta;
            }
            i++;    // next Timer in list
        }
    }
    return count;
}


class MyTimer : public Timer {
public:
    int     i;
    MyTimer(int n, int t=1000, int r=0):Timer(t,r),i(n){};
    MyTimer(const MyTimer& e):i(e.i){coln("MyTimer copied");};
    void callBack(int late);        // callback when Timer fires
};

static const int heartbeat = 13;


void MyTimer::callBack(int late)
{
    digitalWrite( heartbeat, i);
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


//    MyTimer(int n, int t=1000, int r=0):i(n){};
MyTimer     t1( HIGH, 1000, 200 );
MyTimer     t2( LOW,  1020, 200 );

MemTimer    tMem(2000, 2000); // initial delay is 2s and repeat every 2 s

void setup()
{
    Serial.begin(115200);
    pinMode( heartbeat, OUTPUT );
    t1.add();
    t2.add();
    tMem.add();

}

void loop()
{
    Timer::doTimers();
}

