### 1.1 The evils of exceptions

####Exception grouping

Often in exception based code you will see that there are multiple potentially exception throwing functions being protected in a single try block. 

```
try
{
   potentialExceptionFunc1();
   potentialExceptionFunc2();
   ...
}catch( ExceptionName e1 )
{
   // catch block
}
```

There are two problems with this: first, differentiating what should be done whether which function throws is something left to runtime; there's no compile mechanism by which to reliably force the user to differentiate the origin of the exception propagated. Potentially for various functions you different appropriate error handling responses that should be prefered either by convention or practice, but this commonality in code elides this. Second, the lowest bar acceptable of code that can feasibly achieve a functional ends is also the case that you will see in practice. 

####Verbosity

If you didn't commit to some form of exception grouping error, then you segregate your exception handling into individual blocks for the cases where handling must be unique:

```
try
{
   potentialExceptionFunc1();
   ...
}catch( ExceptionName e )
{
   // catch block
}

try
{
   potentialExceptionFunc2();
   ...
}catch( ExceptionName e )
{
   // catch block
}
```

But now you've practically reintroduced the very problem that exceptions were meant to solve, which is that of error handling code isolation. Not to say that Exception Grouping and Verbosity are always the only two pitfalls intertwined.

####Unchecked code path explosion - Cyclomatic Complexity

Each potential place where an exception may be thrown introduces new complexity in terms of execution path and state combination. The worst part about it is that the predominant languages in use does not force the user to handle exceptions. Consider the following example: 

```
try
{
   potentialExceptionFunc1();
   
   allocatedResource = new Something();
   ofstream *blah = new FooBar();
   NetworkWriter *nw = new NetworkWriter();
   
   potentialExceptionFunc2();
   
   uncheckedUseOfAllocatedStuff(allocatedResource, blah, nw);
   goodCleanUp(); // programmers typically think about the complexity for what works
   ...
}catch( ExceptionName e1 )
{
   // not well handled resource management
}
```

In the case of exception grouping as above, there's no compiler enforcement to force the program author to check either at ```uncheckedUseOfAllocatedStuff``` or even at the catch block to ensure that each of the resources are properly allocated or even properly cleaned. Supposing you pick the latter evil of verbosity, you have typically the same scenario in such code-it just moves around because no one actually solved the real problem at hand. Proper error handling in code is directly orthogonal to getting it to operate as you need it to. 

####Catch me if you Can, aka Old Style Exceptions

If you think you had it right by tacking on an additional ```catch (std::exception &ex)``` at the end, think again. Old C code is able to throw numbers and characters and to catch those, you need ```catch (...)``` The problem with this further complicates the standard imposed on language wielders in manifold ways. 

First, you can't know what to expect code to do. You can't trust that a compiler will force you to check for an exception in order that your code be reliable. And you can't even know what to check for when you try to code defensively. While ```catch (...)``` seems to be a panacea, it's actually a crutch, because you can't glean any information about your exception. But if you do code properly, then you add each of the individual primitive types that can be thrown *at every single try block*. 

####Language Nuances Missed by the Masses

*What noexcept truly means* is **not** that a function does not throw an exception because the compiler actually checked that nowhere, recursively throughout all functions called by said function to which ```noexcept``` is applied is there a throw. What it actually means is far, far more sinister. It means that the compiler will brutally throw away any exception checking that it might weave for your beneath that function. C/++ is not a language that puts safety first-no, *speed* comes first. If you want your program to do your bidding at least halfway reliably the onus is on you. What noexcept means is entirely misleading from it's name-it means that istead of propagating an exception up to be caught, it will simply end the program with terminate. I suppose you could derive from this that exceptions don't actually occur.

*The assembler level implementation of exceptions* is frought with peril. Take it from an experienced exploit author-it wasn't just the famously insecure SEH handler that got a pummling from capable attackers. Exceptions require some additional runtime complexity, but whether or not they actually make you any safer is truly a question. The catch 22 here is clear-sure whatever issue might be saved by a correctly authored code fragment that checks exceptions thoroughly is just about nil when it could mean that attackers can just walk through your careful plodding and take everything you have.

*Stack traces: * C++ exceptions aren't like Java's. Java's are nice, and give you stack traces, and even allow the JVM to continue to run if they propagate all the way out so long as there is suitable control flow from which to continue from. If you don't catch a C++ exception you're dead meat, and if you do, well hopefully you get something useful. Maybe you won't.

### 1.2 Variadic types - "or"

####Feasible Alternative to Exceptions

What if, instead of throwing an exception and introducing an unknown control flow, you were to specify what could go wrong in a type, and thus force the function consumer to check what has happened? What if you could express "or" in code, such that either a function returns a valid resource, *or* it returns an error of some kind. The compiler can make sure that you can't get the resource you want without checking it. 

At first this sounds like more burden than you might want. But in reality, you want code that is correct-the burden you pay at compile time is the burden you don't pay when you go try and dig out failure in  your "nuclear reactor code". Boost's Variant does that well. Check out a brief segment of code expressing that either a wrapped C function returns a pointer to validly allocated memory, or some type that represents that memory has not been allocated (error).

```
typedef std::string Error; //can replace the error type with whatever, even void.
template<class T> using CheckedPtr=variant<Error, std::unique_ptr<T>>;
```

To explain, boost's variant is a "compile time type discrimintor"; basically, it's a mechanism that helps you achieve "or". It's somewhat unwieldy to use, because you have to construct class types to facilitate what should happen if it is each type using ```operator()``` overloads. But as you use it and grow, you will learn that the visitor pattern can be succinct too. 



```
template<class T>
CheckedPtr<T> checkedSafeMalloc(size_t size) {
  T * ptr = reinterpret_cast<T*>(malloc(size));
  if (!ptr)
    return CheckedPtr<T>(Error());
  return CheckedPtr<T>(std::unique_ptr<T>(ptr));
}

template<class T>
class my_visitor : public boost::static_visitor<void>
{
  std::function<void(std::unique_ptr<T> &)> &f;
public:
  my_visitor(std::function<void(std::unique_ptr<T> &)> f) : f(f) {
  }
  void operator()(std::unique_ptr<T> &i) const
  {
    f(i);
    std::cout << "validly allocated " << *i.get() << std::endl;
  }
    
  void operator()(const Error &err) const
  {
    std::cout << "Woops! Now handling error appropriately" << std::endl;
  }
};

int main(void) {
  CheckedPtr<int> p = checkedSafeMalloc<int>(20);
  boost::apply_visitor(my_visitor<int>([](std::unique_ptr<int> &uniq_p) {
	*uniq_p.get()=5;
      }), p);
}
```

####Railroad Oriented Programming

I'm actually borrowing someone else's diagram for a moment when I share in the notion of railroad oriented programming. I'm not for the whole paradigm here, because that requires language features that are better implemented in functional languages. But the point in combining compile time checking of variadic types with Railroad Oriented Programming is that you can combine the mechanism and control flow to reduce complexity and support concision in the pursuit of correctness.

```
Need help! How can I best represent this with an example
```

####Correctness



### 1.3 Struct - "and"

The "and" in C++ with types is easy; just use tuple, struct or class. That completes algebraic types-you need only be able to express and & or. The type descriminator works with visitation based upon structs or classes that you've defined just as well as it does with primitives, so there's your completeness.

### 1.4 Completeness and Soundness of Checking

Consider the differences to how practice normally expresses "or" in the programming world:

```
enum {
  case1,
  case2,
  case3
} VARIOUS_POSSIBILITIES;

VARIOUS_POSSIBILITIES someFunc();
...
auto what_happened someFunc();
switch (what_happened) {
    case case1;
        break;
    case case2;
        break;
}
```

The above suffers from subtle problems; VARIOUS_POSSIBILITIES is actually a moniker for int by default, with case1=0 and so on each adding 1. Additionally, if you didn't notice, you declared three cases, but the compiler isn't *guaranteed* to warn you if you don't check for them all. Lastly, proponents of C's speed, would be aghast at the fact that this results in a runtime value check. 

The great part about boost::variant is that it is sound and complete. First, if you try to construct a type visitor on a variant with the wrong member type, notice that you will get an error. Second, notice that you get an error if you do not check all types for which the variant represents. With enum, your code will be happily (wrongly) accepted.

So in summary: use exceptions for truly exceptional situation, such as out-of-ram or other resource or hardware failures. Not typical conditions that are better handled by user code.
