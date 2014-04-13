/*
 * Copyright 2014 Anton Karmanov
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//Garbage Collector.

#include <cassert>
#include <climits>
#include <cstdlib>
#include <iostream>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "dbllist.h"
#include "gc.h"
#include "platform.h"
#include "util.h"

namespace ss = syn_script;
namespace gc = ss::gc;
namespace pf = ss::platform;

using ss::util::ScopeGuard;
using ss::util::UnaryScopeGuard;

namespace {
	const int SIZE_BITS = sizeof(std::size_t) * CHAR_BIT;
	const std::size_t REACHABLE_FLAG = (std::size_t)1 << (SIZE_BITS - 1);
	const std::size_t MOCK_FLAG = (std::size_t)1 << (SIZE_BITS - 2);
	const std::size_t SIZE_MASK = MOCK_FLAG - 1;
}

const std::size_t gc::internal::MAX_SIZE = SIZE_MASK;

namespace {
	const int GC_TIMEOUT_MS = 10000;
	const int GC_TIMEOUT_LIMIT = 6;

	//GC synchronization mutex. Used to synchronize access to the global state.
	std::mutex g_mutex;

	class GCLock {
		NONCOPYABLE(GCLock);

		std::lock_guard<std::mutex> m_guard{ g_mutex };

	public:
		GCLock(){}
	};

	class GCLockEx {
		NONCOPYABLE(GCLockEx);

	public:
		//This member is public, because it has to be passed to wait_for() function.
		std::unique_lock<std::mutex> m_guard{ g_mutex };

		GCLockEx(){}
	};
}

namespace syn_script {
	namespace gc {
		struct ThreadStateLinks;
	}
}

//
//ThreadState
//

//GC state for a particular thread. 
class gc::internal::ThreadState {
	friend struct gc::ThreadStateLinks;

	//Linked list of all thread states.
	ThreadState* m_prev;
	ThreadState* m_next;

	const std::string m_name;
	const bool m_managed;
	bool m_enabled;

	//Non-null, if gc::create() is being executed for this thread.
	void* m_object_being_created;

	//Head of the list of GC roots for this thread (e. g. gc::Local).
	gc::internal::ObjectListElement m_roots_list;

	//Head of the list of objects created in this thread.
	gc::Object m_managed_objects_list;

	pf::Tick_t m_next_sync_tick;

#ifndef NDEBUG
	//Data used to verify that gc_enumerate_refs() enumerates all references defined in a class.
	bool m_checking_references;
	std::vector<const InternalRef*> m_refs_of_new_object;
	std::size_t m_ref_of_new_object_ofs;
#endif

private:
	ThreadState(bool managed, const std::string& name);

public:
	ThreadState(const std::string& name);
	ThreadState(const GlobalState&);
	~ThreadState();

	void list_add_to(ThreadState* list);
	void list_remove_from();
	bool list_is_empty() const;

	const std::string& get_name() const;
	bool is_enabled() const;
	bool is_creating_new_object() const;
	const void* get_object_being_created() const;
	gc::internal::ObjectListElement* get_roots_list();
	gc::Object* get_managed_objects_list();

	void enable();
	void disable();
	void synchronize();

	void suspend_during_garbage_collection(GCLockEx& lock);

	void* new_allocate(std::size_t size);
	void new_finish(gc::Object* object, std::size_t size);
	void new_fail(void* ptr, std::size_t size);

#ifndef NDEBUG
	void add_new_reference(const InternalRef* ref);
	void check_references(Object* object);
	bool check_reference(const InternalRef* ref);
#endif

private:
	void set_enabled(bool enabled);

	typedef UnaryScopeGuard<ThreadState, bool, &ThreadState::set_enabled> SetEnabledGuard;
};

//
//ThreadStateLinks
//

struct gc::ThreadStateLinks {
	typedef ss::DoubleLinkedList<
		internal::ThreadState,
		&internal::ThreadState::m_prev,
		&internal::ThreadState::m_next> ThreadDList0;
};

//
//GlobalState
//

//Global GC state. Only one object of this class exists.
class gc::internal::GlobalState {
public:
	typedef ss::DoubleLinkedList<
		gc::Object,
		&gc::Object::m_list_prev,
		&gc::Object::m_list_next> ObjectDList0;

	typedef ss::DoubleLinkedList<
		gc::internal::ObjectListElement,
		&gc::internal::ObjectListElement::m_prev,
		&gc::internal::ObjectListElement::m_next> ElementDList0;

private:
	bool m_started_up;
	AllocObserver* m_observer;

	std::size_t m_heap_size;
	std::atomic<std::size_t> m_free_heap;

	//true, if gc_enumerate_refs() is being called right now during GC.
	bool m_enumerating_references;

	std::size_t m_threads_count;
	ThreadState m_threads_list;
	std::size_t m_enabled_threads_count;

	//Head of the list of managed objects. On GC, all thread managed objects are moved into this list.
	//Then, reachable objects are moved to the list of reachable objects, and rest are deleted.
	//After that, all reachable objects are moved back to this list.
	gc::Object m_managed_objects_list;

	//Head of the list of reachable objects. Used only during GC.
	gc::Object m_reachable_objects_list;

	//Current value of the reachable flag (the value changes from 0 to 1 and from 1 to 0 on GC, thus
	//reachable objects are marked as unreachable implicitly).
	std::size_t m_reachable_flag;

	bool m_garbage_collection_in_progress;

	//Monitor used for threads synchronization.
	std::condition_variable m_monitor;

public:
	GlobalState();

	bool is_started_up() const;
	std::size_t get_reachable_flag() const;
	AllocObserver* get_observer() const;

	void startup(std::size_t heap_size, AllocObserver* observer);
	void shutdown();
	void collect();
	void synchronize(ThreadState* thread);

	void acquire_memory(ThreadState* thread, std::size_t size);
	void release_memory(std::size_t size);

	void wait_for_garbage_collection_end(GCLockEx& lock);
	void collect_object(Object* object);
	void collect_reference(Object* object);

	void add_managed_thread(ThreadState* thread);
	void remove_managed_thread(ThreadState* thread);
	void add_managed_objects(gc::Object* list_head);
	void thread_enabled(bool enabled);

private:
	template<class Pred> void wait_for(GCLockEx& lock, Pred pred);
	void suspend_enabled_threads(GCLockEx& lock);
	void resume_suspended_threads();

	void collect_synchronized();
	void collect_references();
	void collect_delete_managed_objects();
	void collect_delete(Object* object, std::size_t size);

	bool acquire_memory_try(std::size_t size);

	typedef ScopeGuard<GlobalState, &GlobalState::resume_suspended_threads> ResumeSuspendedThreadsGuard;
};

namespace {
	typedef gc::internal::GlobalState::ObjectDList0 ObjectDList;
	typedef gc::internal::GlobalState::ElementDList0 ElementDList;
	typedef gc::ThreadStateLinks::ThreadDList0 ThreadDList;

	gc::internal::GlobalState g_global_state;
	PLATFORM__THREAD_LOCAL gc::internal::ThreadState* g_thread_state = nullptr;

	void* allocate_memory(std::size_t size) {
		assert(g_global_state.is_started_up());

		void* ptr = operator new(size);
		gc::internal::AllocObserver* observer = g_global_state.get_observer();
		if (observer) observer->memory_allocated(ptr, size);
		return ptr;
	}

	void delete_memory(void* ptr, std::size_t size) {
		assert(g_global_state.is_started_up());

		operator delete(ptr);
		gc::internal::AllocObserver* observer = g_global_state.get_observer();
		if (observer) observer->memory_deleted(ptr, size);
	}
}

//
//ThreadState
//

gc::internal::ThreadState::ThreadState(bool managed, const std::string& name)
: m_prev(this),
m_next(this),
m_name(name),
m_managed(managed),
m_enabled(false),
m_object_being_created(nullptr),
m_roots_list(nullptr),
m_managed_objects_list(true),
m_next_sync_tick(0)
{
#ifndef NDEBUG
	m_checking_references = false;
#endif
}

//Private.
gc::internal::ThreadState::ThreadState(const std::string& name)
: ThreadState(true, name)
{
	GCLock lock;
	assert(g_global_state.is_started_up());
	g_global_state.add_managed_thread(this);
}

gc::internal::ThreadState::ThreadState(const GlobalState&)
: ThreadState(false, std::string("<mock>"))
{}

gc::internal::ThreadState::~ThreadState() {
	if (m_enabled) disable();

	if (m_managed) {
		GCLock lock;
		g_global_state.add_managed_objects(&m_managed_objects_list);
		g_global_state.remove_managed_thread(this);
	}
}

const std::string& gc::internal::ThreadState::get_name() const {
	return m_name;
}

bool gc::internal::ThreadState::is_enabled() const {
	return m_enabled;
}

bool gc::internal::ThreadState::is_creating_new_object() const {
	return !!m_object_being_created;
}

const void* gc::internal::ThreadState::get_object_being_created() const {
	return m_object_being_created;
}

gc::internal::ObjectListElement* gc::internal::ThreadState::get_roots_list() {
	assert(m_managed);
	return &m_roots_list;
}

gc::Object* gc::internal::ThreadState::get_managed_objects_list() {
	assert(m_managed);
	return &m_managed_objects_list;
}

void gc::internal::ThreadState::list_add_to(ThreadState* list) {
	ThreadDList::add(list, this);
}

void gc::internal::ThreadState::list_remove_from() {
	ThreadDList::remove(this);
}

bool gc::internal::ThreadState::list_is_empty() const {
	return ThreadDList::is_empty(this);
}

void gc::internal::ThreadState::enable() {
	assert(m_managed);
	assert(!m_enabled);
	assert(!m_object_being_created);

	GCLockEx lock;

	//If GC is in progress, do not enable the thread and wait for GC end first - there must be
	//no enabled threads during GC.
	g_global_state.wait_for_garbage_collection_end(lock);

	m_next_sync_tick = pf::get_current_tick_count() + pf::GC_SYNC_INTERVAL;
	set_enabled(true);
}

//API.
void gc::internal::ThreadState::disable() {
	assert(m_managed);
	assert(m_enabled);
	assert(!m_object_being_created);

	GCLock lock;
	set_enabled(false);
}

void gc::internal::ThreadState::synchronize() {
	assert(m_managed);
	assert(m_enabled);
	assert(!m_object_being_created);

	pf::Tick_t tick = pf::get_current_tick_count();
	if (tick >= m_next_sync_tick) {
		g_global_state.synchronize(this);
		m_next_sync_tick = pf::get_current_tick_count() + pf::GC_SYNC_INTERVAL;
	}
}

//Disables the thread until the end of GC in order to allow GC to complete. GC can be performed only
//when all other threads are disables.
void gc::internal::ThreadState::suspend_during_garbage_collection(GCLockEx& lock) {
	assert(m_managed);
	assert(m_enabled);
	assert(!m_object_being_created);

	set_enabled(false);
	SetEnabledGuard enable_guard(this, true);
	g_global_state.wait_for_garbage_collection_end(lock);
}

void* gc::internal::ThreadState::new_allocate(std::size_t size) {
	assert(m_managed);
	assert(m_enabled);
	assert(!m_object_being_created);

	if (size > MAX_SIZE) throw out_of_memory();

	std::size_t physical_size = internal::calc_physical_block_size(size);

	g_global_state.acquire_memory(this, physical_size);
	try {
		m_object_being_created = allocate_memory(size);

#ifndef NDEBUG
		m_refs_of_new_object.clear();
		m_ref_of_new_object_ofs = 0;
#endif

		return m_object_being_created;
	} catch (...) {
		g_global_state.release_memory(physical_size);
		throw;
	}
}

void gc::internal::ThreadState::new_finish(gc::Object* object, std::size_t size) {
	assert(m_managed);
	assert(m_object_being_created);

	object->list_add_to(m_managed_objects_list);
	object->manage(size);

	m_object_being_created = nullptr;

#ifndef NDEBUG
	check_references(object);
#endif
}

void gc::internal::ThreadState::new_fail(void* ptr, std::size_t size) {
	assert(m_managed);
	assert(m_object_being_created);

	delete_memory(ptr, size);

	std::size_t physical_size = internal::calc_physical_block_size(size);
	g_global_state.release_memory(physical_size);

	m_object_being_created = nullptr;
}

#ifndef NDEBUG
void gc::internal::ThreadState::add_new_reference(const InternalRef* ref) {
	assert(is_creating_new_object());
	m_refs_of_new_object.push_back(ref);
}

void gc::internal::ThreadState::check_references(gc::Object* object) {
	assert(!m_checking_references);
	m_checking_references = true;
	try {
		object->gc_enumerate_refs();
	} catch (...) {
		m_checking_references = false;
		throw;
	}
	m_checking_references = false;
	assert(m_refs_of_new_object.size() == m_ref_of_new_object_ofs);
	m_refs_of_new_object.clear();
	m_ref_of_new_object_ofs = 0;
}

bool gc::internal::ThreadState::check_reference(const InternalRef* ref) {
	if (!m_checking_references) return false;
	assert(m_ref_of_new_object_ofs < m_refs_of_new_object.size());
	assert(m_refs_of_new_object[m_ref_of_new_object_ofs] == ref);
	++m_ref_of_new_object_ofs;
	return true;
}
#endif

void gc::internal::ThreadState::set_enabled(bool enabled) {
	assert(m_managed);
	assert(m_enabled != enabled);

	m_enabled = enabled;
	g_global_state.thread_enabled(enabled);
}

//
//GlobalState
//

gc::internal::GlobalState::GlobalState()
: m_started_up(false),
m_observer(nullptr),
m_heap_size(0),
m_free_heap(0),
m_enumerating_references(false),
m_threads_count(0),
m_threads_list(*this),
m_enabled_threads_count(0),
m_managed_objects_list(true),
m_reachable_objects_list(true),
m_reachable_flag(0),
m_garbage_collection_in_progress(false)
{}

bool gc::internal::GlobalState::is_started_up() const {
	return m_started_up;
}

std::size_t gc::internal::GlobalState::get_reachable_flag() const {
	return m_reachable_flag;
}

gc::internal::AllocObserver* gc::internal::GlobalState::get_observer() const {
	return m_observer;
}

void gc::internal::GlobalState::startup(std::size_t heap_size, AllocObserver* observer) {
	GCLock lock;

	assert(!m_started_up);
	assert(heap_size > 0);

	m_heap_size = heap_size;
	m_free_heap = heap_size;
	m_observer = observer;
	m_started_up = true;
}

void gc::internal::GlobalState::shutdown() {
	GCLock lock;

	assert(m_started_up);
	assert(m_threads_list.list_is_empty());
	assert(0 == m_threads_count);
	assert(0 == m_enabled_threads_count);
	assert(!m_garbage_collection_in_progress);

	//Delete managed objects - for safety.
	collect_delete_managed_objects();

	assert(m_managed_objects_list.list_is_empty());
	assert(m_heap_size == m_free_heap);

	m_heap_size = 0;
	m_free_heap = 0;
	m_observer = nullptr;
	m_reachable_objects_list.list_clear();
	m_reachable_flag = 0;

	m_started_up = false;
}

void gc::internal::GlobalState::collect() {
	GCLockEx lock;

	assert(g_thread_state);
	assert(g_thread_state->is_enabled());

	if (m_garbage_collection_in_progress) {
		//GC is already in progress in another thread. Disable the current thread, letting GC to complete.
		g_thread_state->suspend_during_garbage_collection(lock);
		return;
	}

	//Perform GC in the current thread.
	suspend_enabled_threads(lock);
	ResumeSuspendedThreadsGuard resume(this);
	collect_synchronized();
}

void gc::internal::GlobalState::collect_synchronized() {
	//Step 1. Move all managed objects from threads to the global list.
	for (ThreadState* thread : ThreadDList(&m_threads_list)) {
		//The size of objects in the global state must have already been incremented.
		add_managed_objects(thread->get_managed_objects_list());
	}

	//Step 2. Process root references.
	for (ThreadState* thread : ThreadDList(&m_threads_list)) {
		for (ObjectListElement* element : ElementDList(thread->get_roots_list())) {
			collect_object(element->m_object);
		}
	}

	//Step 3. Process references of reachable objects.
	collect_references();

	//Step 4. Delete non-referenced objects.
	collect_delete_managed_objects();

	//Step 5. Move objects from the reachable list to the managed list.
	ObjectDList::move_replace(&m_reachable_objects_list, &m_managed_objects_list);

	//Step 6. Invert the reachable flag.
	m_reachable_flag ^= REACHABLE_FLAG;
}

//If the passed object is not marked as reachable, mark it as reachable and move to the reachable list.
void gc::internal::GlobalState::collect_object(Object* object) {
	if (object) {
		if ((object->m_size_and_flags & REACHABLE_FLAG) == m_reachable_flag) {
			object->m_size_and_flags ^= REACHABLE_FLAG;
			object->list_remove_from();
			object->list_add_to(m_reachable_objects_list);
		}
	}
}

void gc::internal::GlobalState::collect_reference(Object* object) {
	assert(m_enumerating_references);
	collect_object(object);
}

//Iterates through all reachable objects and marks all the objects referenced from them as reachable.
void gc::internal::GlobalState::collect_references() {
	m_enumerating_references = true;
	util::BoolGuard guard(&m_enumerating_references);
	for (Object* object : ObjectDList(&m_reachable_objects_list)) {
		object->gc_enumerate_refs();
	}
}

//Deletes unreachable objects.
void gc::internal::GlobalState::collect_delete_managed_objects() {
	std::size_t deleted_size = 0;
	std::size_t deleted_cnt = 0;

	pf::TimeMs_t t = pf::get_current_time_millis();

	auto mng_iter = ObjectDList::begin(&m_managed_objects_list);
	auto mng_end_iter = ObjectDList::end(&m_managed_objects_list);
	while (mng_iter != mng_end_iter) {
		Object* object = *mng_iter;
		++mng_iter;

		std::size_t size = object->get_size();
		collect_delete(object, size);

		std::size_t physical_size = calc_physical_block_size(size);
		deleted_size += physical_size;
		++deleted_cnt;
	}

	m_free_heap += deleted_size;
	t = pf::get_current_time_millis() - t;

	std::cout << "GC: collected " << deleted_cnt << " objects " << deleted_size << " bytes "
		<< t << " ms" << std::endl;
}

void gc::internal::GlobalState::collect_delete(Object* object, std::size_t size) {
	object->list_remove_from();
	object->~Object();
	delete_memory(object, size);
}

void gc::internal::GlobalState::synchronize(ThreadState* thread) {
	GCLockEx lock;
	if (m_garbage_collection_in_progress) thread->suspend_during_garbage_collection(lock);
}

//Acquires a block of memory of the specified size by decreasing the free memory counter.
//If there is not enough free memory, GC is performed. If there is still no free memory,
//an out of memory exception is thrown.
void gc::internal::GlobalState::acquire_memory(ThreadState* thread, std::size_t size) {
	if (acquire_memory_try(size)) return;

	//Not enough free memory. Start GC.

	GCLockEx lock;

	if (m_garbage_collection_in_progress) {
		//GC is already in progress in another thread. Suspend this thread and allow GC to complete.
		thread->suspend_during_garbage_collection(lock);
		if (acquire_memory_try(size)) return;
	}

	//Start GC in the current thread.
	suspend_enabled_threads(lock);
	ResumeSuspendedThreadsGuard resume(this);
	if (!acquire_memory_try(size)) {
		collect_synchronized();
		if (!acquire_memory_try(size)) throw gc::out_of_memory();
	}
}

void gc::internal::GlobalState::release_memory(std::size_t size) {
	m_free_heap += size;
}

bool gc::internal::GlobalState::acquire_memory_try(std::size_t size) {
	std::size_t free_heap = m_free_heap;
	for (;;) {
		if (free_heap < size) return false;
		std::size_t new_free_heap = free_heap - size;
		if (m_free_heap.compare_exchange_strong(free_heap, new_free_heap)) return true;
	}
}

template<class Pred>
void gc::internal::GlobalState::wait_for(GCLockEx& lock, Pred pred) {
	const std::chrono::milliseconds duration(GC_TIMEOUT_MS);

	for (int i = 0; i < GC_TIMEOUT_LIMIT; ++i) {
		bool success = m_monitor.wait_for(lock.m_guard, duration, [pred]{
			return pred();
		});
		if (success) return;

		std::cerr << "GC synchronization timeout" << std::endl;
	}

	//Cannot continue in such state - GC will not work.
	std::cerr << "FATAL ERROR: GC synchronization failed" << std::endl;
	std::abort();
}

//Private.
void gc::internal::GlobalState::wait_for_garbage_collection_end(GCLockEx& lock) {
	wait_for(lock, [this]{
		return !m_garbage_collection_in_progress;
	});
}

//Private.
void gc::internal::GlobalState::suspend_enabled_threads(GCLockEx& lock) {
	assert(!m_garbage_collection_in_progress);
	m_garbage_collection_in_progress = true;

	wait_for(lock, [this] {
		//The caller thread must be the only enabled thread.
		return m_enabled_threads_count == 1;
	});
}

void gc::internal::GlobalState::resume_suspended_threads() {
	assert(m_garbage_collection_in_progress);
	m_garbage_collection_in_progress = false;
	m_monitor.notify_all();
}

void gc::internal::GlobalState::add_managed_thread(ThreadState* thread) {
	thread->list_add_to(&m_threads_list);
	++m_threads_count;
}

void gc::internal::GlobalState::remove_managed_thread(ThreadState* thread) {
	thread->list_remove_from();
	--m_threads_count;
}

void gc::internal::GlobalState::add_managed_objects(gc::Object* list_head) {
	ObjectDList::move_add(list_head, &m_managed_objects_list);
}

void gc::internal::GlobalState::thread_enabled(bool enabled) {
	if (enabled) {
		++m_enabled_threads_count;
	} else {
		--m_enabled_threads_count;
		m_monitor.notify_all();
	}
}

//
//Object
//

gc::Object::Object() : m_size_and_flags(0) {
	assert(g_thread_state);
	assert(g_thread_state->is_enabled());
}

void gc::Object::initialize(){}

gc::Object::Object(bool tag) : m_size_and_flags(MOCK_FLAG) {
	//No checks/synchronization is needed - this constructor is used only by the GC
	//to construct a list head element.
	ObjectDList::init(this);
}

gc::Object::~Object() {
	//This can be called during gc::shutdown(), when there are no threads.
}

void gc::Object::gc_enumerate_refs(){}

void gc::Object::gc_ref_internal(gc::internal::InternalRef& ref) {
#ifdef NDEBUG
	g_global_state.collect_reference(ref.m_object);
#else
	assert(g_thread_state);
	if (!g_thread_state->check_reference(&ref)) {
		g_global_state.collect_reference(ref.m_object);
	}
#endif
}

void gc::Object::list_add_to(Object& list_head) {
	assert(list_head.is_mock());
	assert(!is_mock());
	//The object is not removed from the old list - the caller must take care about that.
	ObjectDList::add(&list_head, this);
}

void gc::Object::list_remove_from() {
	assert(!is_mock());
	//The links of this object are not modified - only the links of the nearby objects.
	ObjectDList::remove(this);
}

bool gc::Object::list_is_empty() const {
	assert(is_mock());
	return ObjectDList::is_empty(this);
}

void gc::Object::list_clear() {
	assert(is_mock());
	ObjectDList::clear(this);
}

void gc::Object::manage(std::size_t size) {
	assert(g_thread_state);
	assert(g_thread_state->is_enabled());
	assert(size <= internal::MAX_SIZE);
	assert(!is_mock());

	//It is the caller's duty to check that GC is enabled for the thread.
	m_size_and_flags = g_global_state.get_reachable_flag() | (size & SIZE_MASK);
}

std::size_t gc::Object::get_size() const {
	return m_size_and_flags & SIZE_MASK;
}

bool gc::Object::is_mock() const {
	return !!(m_size_and_flags & MOCK_FLAG);
}

//
//ObjectListElement
//

gc::internal::ObjectListElement::ObjectListElement(Object* object) {
	m_object = object;
	ElementDList::init(this);
}

gc::internal::ObjectListElement::ObjectListElement(Object* object, ObjectListElement* list_head) {
	m_object = object;
	list_add_to(list_head);
}

void gc::internal::ObjectListElement::list_add_to(ObjectListElement* list_head) {
	//The object is not removed from the old list - the caller must take care about that.
	ElementDList::add(list_head, this);
}

void gc::internal::ObjectListElement::list_remove_from() {
	//The links of this object are not modified - only the links of the nearby objects.
	ElementDList::remove(this);
}

void gc::internal::ObjectListElement::list_clear() {
	ElementDList::clear(this);
}

gc::Object* gc::internal::ObjectListElement::get_object() const {
	return m_object;
}

void gc::internal::ObjectListElement::set_object(Object* object) {
	m_object = object;
}

//
//InternalLocal
//

namespace {
	gc::internal::ObjectListElement* get_thread_roots_list() {
		assert(g_thread_state);
		assert(g_thread_state->is_enabled());
		assert(!g_thread_state->is_creating_new_object());
		return g_thread_state->get_roots_list();
	}
}

gc::internal::InternalLocal::InternalLocal(const Object* object)
: m_list_element(const_cast<Object*>(object), get_thread_roots_list())
{}

gc::internal::InternalLocal::~InternalLocal() {
	assert(g_thread_state);
	assert(g_thread_state->is_enabled());
	assert(!g_thread_state->is_creating_new_object());
	m_list_element.list_remove_from();
}

gc::Object* gc::internal::InternalLocal::internal_get_object() const {
	assert(g_thread_state);
	assert(g_thread_state->is_enabled());
	assert(!g_thread_state->is_creating_new_object());
	return m_list_element.get_object();
}

void gc::internal::InternalLocal::internal_set_object(const Object* object) {
	assert(g_thread_state);
	assert(g_thread_state->is_enabled());
	assert(!g_thread_state->is_creating_new_object());
	assert(!object || !object->is_mock());
	m_list_element.set_object(const_cast<Object*>(object));
}

//
//InternalRef
//

gc::internal::InternalRef::InternalRef() {
	assert(g_thread_state);
	assert(g_thread_state->is_enabled());
	assert(g_thread_state->is_creating_new_object());

#ifndef NDEBUG
	g_thread_state->add_new_reference(this);
#endif

	m_object = nullptr;
}

gc::Object* gc::internal::InternalRef::internal_get_object() const {
	assert(g_thread_state);
	assert(g_thread_state->is_enabled());
	assert(!g_thread_state->is_creating_new_object());
	return m_object;
}

void gc::internal::InternalRef::internal_set_object(const Object* object) {
	assert(g_thread_state);
	assert(g_thread_state->is_enabled());
	assert(!g_thread_state->is_creating_new_object());
	assert(!object || !object->is_mock());
	m_object = const_cast<Object*>(object);
}

//
//manage_thread_guard
//

gc::manage_thread_guard::manage_thread_guard(const std::string& thread_name) {
	assert(!g_thread_state);
	m_thread_state = new gc::internal::ThreadState(thread_name);
	g_thread_state = m_thread_state;
}

gc::manage_thread_guard::~manage_thread_guard() {
	assert(g_thread_state);
	assert(!g_thread_state->is_enabled());
	delete m_thread_state;
	g_thread_state = nullptr;
}

//
//(Functions)
//

void gc::startup(std::size_t heap_size, gc::internal::AllocObserver* observer) {
	g_global_state.startup(heap_size, observer);
}

void gc::shutdown() {
	g_global_state.shutdown();
}

void gc::enable() {
	assert(g_thread_state);
	g_thread_state->enable();
}

void gc::disable() {
	assert(g_thread_state);
	g_thread_state->disable();
}

void gc::collect() {
	g_global_state.collect();
}

void gc::synchronize() {
	assert(g_thread_state);
	g_thread_state->synchronize();
}

void* gc::internal::new_allocate(std::size_t size) {
	assert(g_thread_state);
	return g_thread_state->new_allocate(size);
}

void gc::internal::new_finish(gc::Object* object, std::size_t size) {
	assert(g_thread_state);
	g_thread_state->new_finish(object, size);
}

void gc::internal::new_fail(void* ptr, std::size_t size) {
	assert(g_thread_state);
	g_thread_state->new_fail(ptr, size);
}

std::size_t gc::internal::calc_physical_block_size(std::size_t logical_size) {
	const std::size_t size_size = sizeof(std::size_t);
	static_assert(
		size_size == 2
		|| size_size == 4
		|| size_size == 8
		|| size_size == 16,
		"Wrong sizeof(size_t)");

	const std::size_t size_mask = size_size - 1;

	std::size_t result = (logical_size + size_size * 3 + size_mask) & ~size_mask;
	return result;
}
