#pragma once

/*
 * By Ishaan Govindarajan December 2023 (ported from another project in September 2023)
 * 
 * A handful of general-purpose utility functions for use all around the firmware 
 * 
 */ 


//=========================== CALLBACK FUNCTION HELPERS ========================

/*
 * Callback function "typedefs" essentially
 * I'll start with this: IT'S REEEEEEEAAAAALLLYYYYY DIFFICULT 'CLEANLY' CALL `void()` MEMBER FUNCTIONS OF CLASSES
 * WHILE ALSO MAINTAINING LOW OVERHEAD (i.e. AVOIDING STD::FUNCTION)
 * 	\--> Re: this latter point, people claim that std::function requires heap usage and a lotta extra bloat
 *
 * As such, I've defined three basically container classes that can hold + call callback functions
 *
 * 	>>> `Callback Function` is the most generic of these types
 * 		- Lets you attach a global-scope c-style function or any kinda static function
 * 		- Default constructor is "safe" i.e. calling an uninitialized `Callback Function` will do nothing rather than seg fault
 * 		- call the callback using the standard `()` operator syntax
 *
 * 	>>> `Instance Callback Function` is slightly more specialized:
 * 		- Lets you call a instance method of a particular class on a provided instance
 * 		- Default constructor should be "safe" i.e. calling an uninitialized `Instance Callback Function` will do nothing
 * 			\--> this may incur a slight performance penalty, but safety may be preferred here
 * 			\--> I'm hoping some kinda compiler optimization happens here, but ehhh we'll see
 * 		- call the `instance.callback()` using the standard `()` operator syntax
 *
 * 	>>> `Context Callback Function` is the most generic
 * 		- Lets you attach a global-scope c-style function or any kinda static function
 * 		- but also [OPTIONALLY] pass a generic pointer to some `context` to the function
 * 			\--> Anticipated to use the `context` field to pass an instance of a class (or some kinda struct)
 * 		- anticipated use is with a forwarding function that takes a `void*` or `<Type>*` argument
 * 			\--> it will `static_cast()` the `context` back to the intended type, then call one of its instance methods
 * 		- default constructor is "safe" i.e. calling an uninitialized `Context Callback Function` will do nothing rather than seg fault
 * 		- call the callback using the standard `()` operator syntax
 *
 * All the `()` operators are aggressively optimized to minimize as much overhead as possible
 *
 * Insipred by this response in a PJRC forum:
 * https://forum.pjrc.com/threads/70986-Lightweight-C-callbacks?p=311948&viewfull=1#post311948
 * and this talk (specifically at this timestamp):
 * https://youtu.be/hbx0WCc5_-w?t=412
 */

#include <array> //for span implementation
#include <Arduino.h> //don't think this is necessary, but just outta good form

class Callback_Function {
public:
	static inline void empty_cb() {} //upon default initialization, just point to this empty function
	Callback_Function(void(*_func)(void)): func(_func) {}
	Callback_Function(): func(empty_cb) {}
	void __attribute__((optimize("O3"))) operator()() {func();}
	void __attribute__((optimize("O3"))) operator()() const {func();}
private:
	void(*func)(void);
};


template<typename T> //need to specialize a particular target class
class Instance_Callback_Function {
public:
	Instance_Callback_Function(): instance(nullptr), func(nullptr) {}
	Instance_Callback_Function(T* _instance, void(T::*_func)()): instance(_instance), func(_func) {}
	void __attribute__((optimize("O3"))) operator()() {if(instance != nullptr)  ((*instance).*func)();}
	void __attribute__((optimize("O3"))) operator()() const {if(instance != nullptr) ((*instance).*func)();}
private:
	T* instance;
	void(T::*func)();
};


template<typename T = void> //defaults to generic type
class Context_Callback_Function {
public:
	static inline void empty_cb(T* context) {} //upon default initialization, just point to this empty function
	Context_Callback_Function(void(*_no_context_func)(void)): context((T*)nullptr), func(empty_cb), no_context_func(_no_context_func) {} //let us pass in a function without context
	Context_Callback_Function(T* _context, void(*_func)(T*)): context(_context), func(_func), no_context_func(nullptr) {} //we want a context type callback function
	Context_Callback_Function(): context((T*)nullptr), func(empty_cb), no_context_func(nullptr) {} //default constructor, run the empty callback when called
	void __attribute__((optimize("O3"))) operator()() {if(no_context_func == nullptr) (*func)(context); else no_context_func();}
	void __attribute__((optimize("O3"))) operator()() const {if(no_context_func == nullptr) (*func)(context); else no_context_func();}
private:
	T* context;
	void(*func)(T*);
	void(*no_context_func)(void); //have this so we can call regular callback functions without any funny business
};

/*
 * ====================== "DIY" SPAN IMPLEMENTATION ===================
 * In previous projects I've found myself using std::span pretty often instead of c-style array constructs
 * Unfortunately, std::span is only available on C++ compilers 2017 and later, and arduino defaults to C++ 2014
 * 
 * The fundamentals of std::span aren't super complicated--it's essentially holds a pointer to an array and its size in a single structure
 * And some other entity actually owns the array. It shouldn't be too difficult to get a basic implementation going 
 * 
 * TODO:
 * 	- Handle const span
 */

template<typename T>
class App_Span {
public:
	//a couple types of constructors
	App_Span(): span_ptr(nullptr), span_size(0) {} //default constructor
	App_Span(T* _ptr, size_t _size): span_ptr(_ptr), span_size(_size) {}; //constructor with c-style interface

	//special templatized constructor for std::span
	template<size_t len>
	App_Span(std::array<T, len>& arr): span_ptr(arr.data()), span_size(arr.size()) {} //constructor with std::span

	//override some operators to provide an array-style interface
	//no bounds checking for simplicity
	T operator [](int i) const { return span_ptr[i]; }
	T& operator [](int i) { return span_ptr[i]; }

	//provide stl-style interfaces for iterators and such
	//code generation assisted by ChatGPT lol
	inline size_t size() { return span_size; }
	inline T* begin() { return span_ptr; }
	inline T* end() { return span_ptr + span_size; }

private:
	//just hold a pointer and a size
	T* span_ptr;
	size_t span_size; 
};