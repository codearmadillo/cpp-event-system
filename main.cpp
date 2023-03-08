#include <iostream>
#include <functional>
#include <queue>

#define BIT(x) 1 << x


enum EventType {
    None = 0,
    KeyPressed = BIT(0), KeyReleased = BIT(1)
};
struct Event {
    explicit Event(EventType type) : m_type(type) { }
    [[nodiscard]] inline EventType get_type() const {
        return m_type;
    }
    [[nodiscard]] inline bool is_type(EventType type) const {
        return m_type == type;
    }
    private:
        EventType m_type;
};


struct IEventHandler;
struct IEventBus {
    public:
        virtual void push_to_queue(Event&& event) = 0;
        virtual void process_queue() = 0;
    private:
        virtual int register_handler(IEventHandler* handler) = 0;
        virtual void unregister_handler(int handlerId) = 0;
};
struct IEventHandler {
    virtual bool handle(const Event& event) = 0;
    [[nodiscard]] virtual inline int get_signature() const = 0;
};


struct EventBus : public IEventBus {
    public:
        void push_to_queue(Event&& event) override {
            m_queue.emplace(event);
        }
        void process_queue() override {
            while(!m_queue.empty()) {
                auto event = m_queue.front();
                for (int i = 0; i < m_handlersAlive; i++) {
                    auto handler = m_handlers.at(i);
                    if (handler->get_signature() & event.get_type()) {
                        auto stop_propagation = handler->handle(event);
                        if (stop_propagation) {
                            break;
                        }
                    }
                }
                m_queue.pop();
            }
        }
        static EventBus& get_instance() {
            static EventBus instance;
            return instance;
        }
        EventBus(EventBus const&) = delete;
        void operator=(EventBus const&) = delete;
        int register_handler(IEventHandler* eventHandler) override {
            // This will not work!
            // If I have 3 elements and I remove the one in the middle, suddenly the handler has invalid reference
            m_handlers.insert({ m_handlersAlive, eventHandler });
            m_handlersAlive++;
        }
        void unregister_handler(int handlerId) override {
            m_handlers.erase(handlerId);
            m_handlersAlive--;
        }
    private:
        EventBus() = default;
    private:
        int m_handlersAlive {0};
        std::unordered_map<int, IEventHandler*> m_handlers;
        std::queue<Event> m_queue {};
};
struct EventHandler : public IEventHandler {
    explicit EventHandler(int handlerSignature): m_handlerSignature(handlerSignature) {
        m_handlerId = EventBus::get_instance().register_handler(this);
    }
    ~EventHandler() {
        EventBus::get_instance().unregister_handler(m_handlerId);
    }
    [[nodiscard]] inline int get_signature() const override {
        return m_handlerSignature;
    }
    private:
        int m_handlerId;
        int m_handlerSignature;
};



// Example
struct KeyPressEvent : public Event {
    KeyPressEvent() : Event(EventType::KeyPressed) { }
};
struct Actor : public EventHandler {
    Actor(): EventHandler(EventType::KeyPressed | EventType::KeyReleased) { }
    bool handle(const Event& event) final {
        // only events of type EventType::KeyPressed/EventType::KeyReleased will be handled here
        if (event.is_type(EventType::KeyPressed)) {
            std::cout << "Hey! You pressed a key!\n";
        }
        return true;
    }
};




int main() {

    // Create event
    KeyPressEvent event;
    // Create actor
    Actor actor;
    Actor actor2;
    // Dispatch event
    EventBus::get_instance().push_to_queue(std::move(event));
    // Process queue
    // First actor should react, second actor should not as propagation will stop
    EventBus::get_instance().process_queue();

    return 0;
}
