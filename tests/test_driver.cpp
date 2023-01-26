// https://stackoverflow.com/questions/32066204/construct-path-for-include-directive-with-macro

#define IDENT(x) x
#define XSTR(x) #x
#define STR(x) XSTR(x)
#define PROXY(x,y) STR(IDENT(x)IDENT(y))
#include PROXY(WHICH_TEST, _generated.hpp)

int main() {
  staged();
}
