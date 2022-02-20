#define PARCER_IMPL  // otherwise non-template implementation won't generate, only import headers
#include "parcer.h"
#include <iostream>

using namespace parcer;

void test(std::string inp){
  auto helloP = KWParser::mk("Hello") << many(CharParser::mk(' '));
  auto worldP = KWParser::mk("World") << many(CharParser::mk(' '));
  auto p = helloP >> worldP << eof();
  auto ans = p.parse(inp, 0);
  std::cout << inp << ": " << ans.is_none << " " << ans.pos << " " << ans.t << " " << ans.err << std::endl;
}

int main(){
  test("HelloWorld");
  test("Hello  World   ");
  test("Hello World!");
  return 0;
}
