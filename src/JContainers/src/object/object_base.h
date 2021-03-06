#pragma once

#include <mutex>
#include <atomic>
#include <assert.h>
#include <boost/optional/optional.hpp>
#include "boost/noncopyable.hpp"

#include "intrusive_ptr.hpp"
#include "util/spinlock.h"
#include "util/istring.h"

namespace collections {

    typedef UInt32 HandleT;

    enum class Handle : HandleT {
        Null = 0,
    };

    class object_base;
    class object_context;

    enum CollectionType {
        None = 0,
        Array,
        Map,
        FormMap,
        IntegerMap,
    };

    struct object_base_stack_ref_policy {
        static void retain(object_base * p);
        static void release(object_base * p);
    };

    template <class T>
    using object_stack_ref_template = boost::intrusive_ptr_jc<T, object_base_stack_ref_policy>;
	using object_stack_ref = object_stack_ref_template<object_base>;
	using spinlock = util::spinlock;

    class object_base : public boost::noncopyable
    {
        //object_base(const object_base&);
        //object_base& operator=(const object_base&);

        friend class object_context;
    public:
        typedef uint32_t time_point;

    public:
        std::atomic<Handle> _id                 = Handle::Null;

        std::atomic_int32_t _refCount           = 0;
        std::atomic_int32_t _tes_refCount       = 0;
        std::atomic_int32_t _stack_refCount     = 0;
        std::atomic_int32_t _aqueue_refCount    = 0;
        time_point _aqueue_push_time            = 0;

        CollectionType                          _type = CollectionType::None;
        util::istring                           _tag;
    private:
        object_context *_context                = nullptr;

        void release_counter(std::atomic_int32_t& counter);
        bool is_completely_initialized() const { return _context != nullptr; }
        void try_prolong_lifetime();

    public:

        virtual ~object_base() {}

    public:
        using lock = std::lock_guard<spinlock>;
        mutable spinlock _mutex;

        explicit object_base(CollectionType type)
            : _type(type)
        {
        }

        // for test purpose only!
        // registers (or returns already registered) identifier
		// never prolongs lifetime (unlike @uid() func.)
        Handle public_id();

        CollectionType type() const { return _type; }
        Handle _uid() const {  return _id.load(std::memory_order_relaxed); }
		Handle uid();

        bool is_public() const {
            return _uid() != Handle::Null;
        }

        spinlock& mutex() const { return _mutex; }

        template<class T> T* as() {
            return const_cast<T*>(const_cast<const object_base*>(this)->as<T>());
        }

        template<class T> const T* as() const {
            return (this && T::TypeId == _type) ? static_cast<const T*>(this) : nullptr;
        }

        template<> const object_base* as<object_base>() const {
            return this;
        }

        template<class T> T& as_link() {
            return const_cast<T&>(const_cast<const object_base*>(this)->as_link<T>());
        }

        template<class T> const T& as_link() const {
            const T* casted_object = this->as<T>();
            assert(casted_object != nullptr && "can't cast the object");
            return *casted_object;
        }

        object_base * retain() {
            ++_refCount;
            return this;
        }

        object_base * tes_retain();

        int32_t refCount() const {
            return _refCount + _tes_refCount + _stack_refCount + _aqueue_refCount;
        }
        bool noOwners() const {
            return
                _refCount.load() <= 0 &&
                _tes_refCount.load() <= 0 &&
                _aqueue_refCount.load() <= 0 &&
                _stack_refCount.load() <= 0;
        }

        bool u_is_user_retains() const {
            return _tes_refCount.load(std::memory_order_relaxed) > 0;
        }
        bool is_in_aqueue() const {
            return _aqueue_refCount.load(std::memory_order_relaxed) > 0;
        }

        // push the object into the queue (which will own it temporarily)
        object_base * prolong_lifetime();
        object_base * zero_lifetime();

        void release();
        void tes_release();
        void stack_retain() { ++_stack_refCount; }
        void stack_release();

        // releases and then deletes object if no owners
        // true, if object deleted
        void _aqueue_retain() { ++_aqueue_refCount; }
        bool _aqueue_release();
        void _delete_self();

        void set_context(object_context & ctx) {
            jc_assert(!_context);
            _context = &ctx;
        }

        object_context& context() const {
            jc_assert(_context);
            return *_context;
        }

        void _registerSelf();

        virtual void u_clear() = 0;
        virtual SInt32 u_count() const = 0;
        virtual void u_onLoaded() {};

        // nillify object cross references to avoid high-level
        // release calls and resulting deadlock
        virtual void u_nullifyObjects() = 0;

        SInt32 s_count() const {
            lock g(_mutex);
            return u_count();
        }

        void s_clear() {
            lock g(_mutex);
            u_clear();
        }

        void set_tag (const char* tag)
        {
            lock g (_mutex);
            if (tag) _tag = tag;
            else _tag.clear ();
        }

        bool has_equal_tag (char const* tag) const
        {
            if (tag)
            {
                lock g (_mutex);
                return _tag == tag;
            }
            return false;
        }

        virtual void u_visit_referenced_objects(const std::function<void(object_base&)>& visitor) {}
    };

    inline void object_base_stack_ref_policy::retain(object_base * p) {
        p->stack_retain();
    }

    inline void object_base_stack_ref_policy::release(object_base * p) {
        p->stack_release();
    }

    struct internal_object_lifetime_policy {
        static void retain(object_base * p) {
            p->retain();
        }

        static void release(object_base * p) {
            p->release();
        }
    };

    typedef boost::intrusive_ptr_jc<object_base, internal_object_lifetime_policy> internal_object_ref;


    class object_lock {
        object_base::lock _lock;
    public:
        explicit object_lock(const object_base *obj) : _lock(obj->_mutex) {}
        explicit object_lock(const object_base &obj) : _lock(obj._mutex) {}

        template<class T, class P>
        explicit object_lock(const boost::intrusive_ptr_jc<T, P>& ref) : _lock(static_cast<const object_base&>(*ref)._mutex) {}
    };
}
