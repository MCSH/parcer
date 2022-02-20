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

void test2(std::string inp){
  auto helloP = KWParser::mk("Hello");
  auto worldP = KWParser::mk("World");
  auto sep = many(CharParser::mk(' ')) >> CharParser::mk(',') >> many(CharParser::mk(' '));
  auto p = inside(CharParser::mk('['), seperated(helloP || worldP, sep), CharParser::mk(']'));
  auto ans = p.parse(inp, 0);
  std::cout << inp << ": " << inp.size() << " " << ans.is_none << " " << ans.pos << " " << ans.t[0] << " " << ans.err << std::endl;
}

int main(){
  test("HelloWorld");
  test("Hello  World   ");
  test("Hello World!");

  test2("[Hello, World  ,   Hello]");
  return 0;
}
