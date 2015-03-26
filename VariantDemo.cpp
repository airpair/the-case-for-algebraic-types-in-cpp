#include <memory>
#include <functional>
#include <iostream>
#include <exception>

#include <boost/variant.hpp>

using boost::variant;

typedef std::string Error;

template<class T> using CheckedPtr=variant<Error, std::unique_ptr<T>>;

template<class T> CheckedPtr<T> checkedSafeMalloc(size_t size) {
  T *ptr=reiterpret_cast<T*>(malloc(size));
  if (!ptr)
    return CheckedPtr<T>(Error());
  return CheckedPtr<T>(std::unique_ptr<T>(ptr));
}

template<class T>
class PtrVisitor : public boost::static_visitor<void>
{
  std::function<void(std::unique_ptr<T> &)> &f;
public:
  PtrVisitor(std::function<void(std::unique_ptr<T> &)> f) : f(f) {}
  void operator()(std::unique_ptr<T> &i) const {
    f(i);
    std::cout << "validly allocated" << *i.get() << std::endl;
  }

  void operator()(const Error &err) const {
    std::cout << "woops! Now handling error appropriately" << std::endl;
  }
};

enum {
  case1,
  case2
} POSSIBILITIES;

int main(int argc, char **argv) {
  CheckedPtr<int> p = checkedSafeMalloc<int>(20);
  boost::apply_visitor(PtrVisitor<int>([](std::unique_ptr<int> &uniq_p) {
	* uniq_p.get()=5;
      }), p);
}
