#include <iostream>
#include <functional>
#include <queue>
#include <memory>
#include <cassert>

#define BIT(x) 1 << x

template<typename T>
struct ListNode {
    T* value;
    struct ListNode<T>* next;
};


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
        virtual ListNode<IEventHandler>* register_handler(IEventHandler* handler) = 0;
        virtual void unregister_handler(ListNode<IEventHandler>* node) = 0;
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
            if (m_head == nullptr) {
                return;
            }
            while (!m_queue.empty()) {
                auto event = m_queue.front();
                auto tail = m_head;
                while (tail != nullptr) {
                    auto handler = tail->value;
                    if (handler->get_signature() & event.get_type()) {
                        auto stop_propagation = handler->handle(event);
                        if (stop_propagation) {
                            break;
                        }
                    }
                    // move to next handler
                    tail = tail->next;
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
        ListNode<IEventHandler>* register_handler(IEventHandler* eventHandler) override {
            auto node = new ListNode<IEventHandler> { eventHandler, nullptr };
            // check if handler is new head
            if (m_head == nullptr) {
                m_head = node;
            } else {
                auto tail = m_head;
                while (tail->next != nullptr) {
                    tail = tail->next;
                }
                tail->next = node;
            }
            // return node
            return node;
        }
        void unregister_handler(ListNode<IEventHandler>* node) override {
            assert(m_head != nullptr && "Something went wrong - the handler list is null");
            // find parent of node
            auto tail = m_head;
            while (tail->next != node) {
                tail = tail->next;
            }
            // now we have parent - yay! change the reference to connect the list
            tail->next = node->next;
            // the listnode is not "detached" and can be removed manually. the data is destroyed (as the IEventHandler unregisters in destructor)
            delete node;
        }
    private:
        EventBus() = default;
    private:
        ListNode<IEventHandler>* m_head { nullptr };
        std::queue<Event> m_queue {};
};
struct EventHandler : public IEventHandler {
    explicit EventHandler(int handlerSignature): m_handlerSignature(handlerSignature) {
        m_node = EventBus::get_instance().register_handler(this);
    }
    ~EventHandler() {
        EventBus::get_instance().unregister_handler(m_node);
    }
    [[nodiscard]] inline int get_signature() const override {
        return m_handlerSignature;
    }
    private:
        ListNode<IEventHandler>* m_node;
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
    auto actor = new Actor();
    auto actor2 = new Actor();
    auto actor3 = new Actor();

    // Dispatch event
    EventBus::get_instance().push_to_queue(std::move(event));

    // Process queue
    // First actor should react, second actor should not as propagation will stop
    EventBus::get_instance().process_queue();

    return 0;
}
