//namespace efl { // event framework library

/**
 * LL is a template class which adds the linked list to objects of type Item
 * which are the various Event types. It use a singly linked list to minimize
 * RAM footprint. Lists are expected to be relatively short so traversing the
 * list from the start to find the previous element it should mot take too
 * long. It uses a sentinel to point tothe first real item in the list and the
 * last item in the list points to the sentinel.
 */

template<class Item> class LL {
private:
  LL* 	    pNext;          // point to next item in list
  static LL<Item>& sentinel() {
    static LL<Item> rc=LL<Item>((Item*)0);
    return rc;
  };
  Item*    pItem;                 // event descriptor
  LL():
  pItem((Item*)0),pNext(this) {
  }; // can't construct w/out an Item except for the special case in sentinel()
public:
  // return status of list manipulation operations
  enum rs {
    OK=0,                   // return status - operation succeeded
    NAK=1,                  // general bad status
    BAD_DUP=2               // duplicate e.g already in a list.
  };

  LL(Item*pI):
  pNext(this),pItem(pI) {
  };   // construct from an Item
  rs add();                               // add an Item to the tail of the queue
  rs push();                              // push an Item on to he front of the queue
  LL* next() {
    return pNext;
  };              // return pointer to next iutem in list
  LL* previous();                         // pointer to previous item in list
  static LL* begin() {
    return sentinel().pNext;
  };   // pointer to first item in list (past the sentinel)
  static LL* end() {
    return &sentinel();
  };  // pointer to end of list (e.g. the sentinel)
  LL* erase();                            // remove this item from the list
  static int& size() {
    static int elementCount=0;
    return elementCount;
  };
  static void doItems();      // each specialization expected to provide their own 'doItems()'

#if !defined AVR
  static void walk();
#endif
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
  } 
  else {
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

#if !defined AVR

template<class Item>
void LL<Item>::walk()
{
  coln("");
  co("walk(");
  co(size());
  co(") ");
  LL<Item>*  pLL = LL<Item>::sentinel().pNext;
  co( (void*)&sentinel() );
  co( " -> ");
  while( pLL != &LL<Item>::sentinel() )
  {
    co( pLL);
    co( " -> ");
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

/*
 * Generic Event - one can just chain a bunch of these together and
 * execute them. Not very interesting but the simplest case.
 */
class Event { // callback makes this a one shot event (won't execute until it is added back to the list.)
public:
  Event() {
  };
  virtual bool callback() {
    if (verbose) coln( "Event:");
    return false;
  };
};

template<>
void LL<Event>::doItems()
{
  for(LL<Event>* pLL = begin(); pLL != end(); )
    if(!pLL->pItem->callback())
      pLL = pLL->erase();         // remove from list
}


/*
 * Timer class gets a little more interesting. The default behavior
 * is a one shot but given a non-zero period will be periodic.
 *
 */
class Timer {
private:
  uLong   counter;
  uLong   period;
public:
  Timer(unsigned long c=1, unsigned long p=0):
  counter(c),period(p) {
  }; // default to fire once after 1 ms
  virtual bool callback(uLong late) {
    if (verbose) {
      coln( "Timer:");
    }
    return false;
  };
  uLong getCounter() {
    return counter;
  }
  void setCounter(uLong c) {
    counter=c;
  }
  uLong getPeriod() {
    return period;
  }
  void setPeriod(uLong p) {
    period=p;
  }

};

template<>
void LL<Timer>::doItems()
{
  static uLong prevMillis = 0;    // start when execution starts
  uLong   nowMillis = millis();   // time now
  uLong   deltaMillis = nowMillis - prevMillis;

  if(!deltaMillis)
    return;

  // iterate through timers to see which ones have downconted to or beyond zero
  for(LL<Timer>* pLL = begin(); pLL != end(); )
  {
    uLong late=deltaMillis - pLL->pItem->getCounter();
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
  }
  prevMillis = nowMillis;
}
//#define DIGITAL
#if defined DIGITAL

/**
 * Provide a framework to respond to changes in state of digital inputs including 
 * debouncing the input for a specified time (in milliseconds.)
 */

class Digital { // Monitor a digital input
public:
  typedef enum {
    INACTIVE,
    GOING_ACTIVE,
    ACTIVE,
    GOING_INACTIVE,
  } 
  States;

  typedef enum {
    ACT_HI,
    ACT_LO,
  } 
  Polarity;

private:    
  uLong       debounce;
  States      state;
  Polarity    polarity;

public:
  Digital(unsigned long d=1, Polarity p=ACT_HI):
  debounce(d),polarity(p) {
  }; // default to fire once after 1 ms
  virtual bool callback(uLong late) {
    if (verbose) {
      coln( "Digital:");
    }
    return false;
  };

};

/*
template<>
 void LL<Timer>::doItems()
 {
 static uLong prevMillis = 0;    // start when execution starts
 uLong   nowMillis = millis();   // time now
 uLong   deltaMillis = nowMillis - prevMillis;
 
 if(!deltaMillis)
 return;
 
 if(verbose) walk();
 
 // iterate through timers to see which ones have downconted to or beyond zero
 for(LL<Timer>* pLL = begin(); pLL != end(); )
 {
 uLong late=deltaMillis - pLL->pItem->getCounter();
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
#endif //defined DIGITAL

//} // namespace efl


