

enum Event {

};

class EventHandler {
public:
    virtual void handleEvents(int events) = 0;

    virtual HANDLE getHandle() = 0;
};