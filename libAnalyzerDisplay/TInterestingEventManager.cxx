#include "TInterestingEventManager.hxx"


// Allocating and initializing GlobalClass's
// static data member.  
TInterestingEventManager* TInterestingEventManager::s_instance = 0;

TInterestingEventManager::TInterestingEventManager(){

  fEnabled = false;
  fInterestingEvent = false;
 
};


TInterestingEventManager* TInterestingEventManager::instance()
{
  if (!s_instance)
    s_instance = new TInterestingEventManager();
  return s_instance;
}
