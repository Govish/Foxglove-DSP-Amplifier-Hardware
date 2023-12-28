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


template<typename T> //defaults to generic type
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