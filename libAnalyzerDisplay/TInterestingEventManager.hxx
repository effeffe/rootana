#ifndef TInterestingEventManager_hxx_seen
#define TInterestingEventManager_hxx_seen

/// Singleton class for defining which events are interesting (and should be plotted).
/// User needs to enable use of  TInterestingEventManager in program constructor
///  
/// 
class TInterestingEventManager
{
public:
  
  static TInterestingEventManager *instance();

  /// Enable the InterestingEventManager
  /// Needs to be done in constructor of the program.
  void Enable(){fEnabled = true;}

  /// Check if manager is enabled
  bool IsEnabled(){return fEnabled;}

  /// Set this event to be interesting
  void SetInteresting(){fInterestingEvent = true;}

  /// Is this event interesting?
  bool IsInteresting(){return fInterestingEvent;}

  /// Reset state of manager = set to not interesting event.
  void Reset(){ fInterestingEvent = false;}

private:
  
  // pointer to global object
  static TInterestingEventManager *s_instance;

  // hidden private constructor
  TInterestingEventManager();
  
  // enabled bool
  bool fEnabled;

  // interesting event bool
  bool fInterestingEvent;   
  
};

//#define  iem_t  TInterestingEventManager;
// Setup a short name for class IEM = Interesting Event Manager
typedef TInterestingEventManager iem_t;


#endif


