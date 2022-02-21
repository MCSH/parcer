#pragma ocne
#include <memory>
#include <string>
#include <vector>

#ifndef PREPARCE_CALLBACK
#define PREPARCE_CALLBACK
#endif

namespace parcer {
using ERROR = std::string;
using Input = std::string &;
template <class T> using Container = std::shared_ptr<T>;
template <class T> using List = std::vector<T>;

template <class T> struct PResult {
  T t;
  bool is_none;
  int pos;
  ERROR err;
};

template <class T> class ParseUnit {
public:
  virtual ~ParseUnit() = default;
  virtual PResult<T> parse(Input, int pos) = 0;
};

template <class T> class Parser {
public:
  Container<ParseUnit<T>> p;
  Parser(Container<ParseUnit<T>> p) : p(p) {}
  PResult<T> parse(Input inp, int pos) {
    PREPARCE_CALLBACK
    return p->parse(inp, pos);
  }
};

template <class TT, class T> class ParserPipe : public ParseUnit<T> {
  Container<ParseUnit<TT>> p1;
  Container<ParseUnit<T>> p2;

public:
  virtual PResult<T> parse(Input inp, int pos) override {
    PREPARCE_CALLBACK
    auto r1 = p1->parse(inp, pos);
    if (r1.is_none) {
      PResult<T> out;
      out.is_none = true;
      out.err = r1.err;
    }
    return p2->parse(inp, r1.pos);
  }
  ParserPipe(Container<ParseUnit<TT>> p1, Container<ParseUnit<T>> p2)
      : p1(p1), p2(p2) {}
};

template <class TT, class T> Parser<T> operator>>(Parser<TT> p1, Parser<T> p2) {
  return Container<ParseUnit<T>>(new ParserPipe<TT, T>(p1.p, p2.p));
}

template <class TT, class T> class RevParserPipe : public ParseUnit<TT> {
  Container<ParseUnit<TT>> p1;
  Container<ParseUnit<T>> p2;

public:
  virtual PResult<TT> parse(Input inp, int pos) override {
    PREPARCE_CALLBACK
    auto r1 = p1->parse(inp, pos);
    if (r1.is_none) {
      return r1;
    }
    auto r2 = p2->parse(inp, r1.pos);
    r1.pos = r2.pos;
    r1.err = r2.err;
    return r1;
  }
  RevParserPipe(Container<ParseUnit<TT>> p1, Container<ParseUnit<T>> p2)
      : p1(p1), p2(p2) {}
};

template <class TT, class T>
Parser<TT> operator<<(Parser<TT> p1, Parser<T> p2) {
  return Container<ParseUnit<TT>>(new RevParserPipe<TT, T>(p1.p, p2.p));
}

template <class T> class ParserOr : public ParseUnit<T> {
  Container<ParseUnit<T>> p1;
  Container<ParseUnit<T>> p2;

public:
  virtual PResult<T> parse(Input inp, int pos) override {
    PREPARCE_CALLBACK
    auto r1 = p1->parse(inp, pos);
    if (r1.is_none)
      return p2->parse(inp, pos);
    return r1;
  }
  ParserOr(Container<ParseUnit<T>> p1, Container<ParseUnit<T>> p2)
      : p1(p1), p2(p2) {}
};

template <class T> Parser<T> operator||(Parser<T> p1, Parser<T> p2) {
  return Container<ParseUnit<T>>(new ParserOr<T>(p1.p, p2.p));
};

#ifndef NO_KW_PARCERS
class KWParser : public ParseUnit<std::string> {
  std::string kw;
  int len;

public:
  KWParser(std::string);
  static Parser<std::string> mk(std::string);
  virtual PResult<std::string> parse(Input, int pos) override;
};

#ifdef PARCER_IMPL
KWParser::KWParser(std::string kw) : kw(kw) { len = kw.size(); }

PResult<std::string> KWParser::parse(Input inp, int pos) {
  PREPARCE_CALLBACK
  if (inp.substr(pos, len) == kw)
    return PResult<std::string>{kw, false, pos + len, ""};
  else
    return PResult<std::string>{kw, true, pos, "Expecting " + kw};
}

Parser<std::string> KWParser::mk(std::string inp) {
  return Container<ParseUnit<std::string>>(new KWParser(inp));
}
#endif
#endif

#ifndef NO_CHAR_PARCER
class CharParser : public ParseUnit<char> {
  char c;

public:
  CharParser(char c) : c(c) {}
  static Parser<char> mk(char c);
  virtual PResult<char> parse(Input, int pos) override;
};
#ifdef PARCER_IMPL
Parser<char> CharParser::mk(char c) {
  return Container<ParseUnit<char>>(new CharParser(c));
}

PResult<char> CharParser::parse(Input inp, int pos) {
  PREPARCE_CALLBACK
  if (inp[pos] == c) {
    return PResult<char>{c, false, pos + 1, ""};
  }
  return PResult<char>{c, true, pos, "Expecting " + c};
}
#endif
#endif

#ifndef NO_MANY_PARCER
template <class T> class ManyParser : public ParseUnit<List<T>> {
  Container<ParseUnit<T>> baseP;

public:
  ManyParser(Container<ParseUnit<T>> baseP) : baseP(baseP) {}
  virtual PResult<List<T>> parse(Input inp, int pos) override {
    PREPARCE_CALLBACK
    List<T> out{};
    int npos = pos;
    do {
      auto o = baseP->parse(inp, npos);
      pos = npos;
      npos = o.pos;
      out.push_back(o.t);
    } while (npos > pos);
    return PResult<List<T>>{out, false, npos};
  }
};

template <class T> Parser<List<T>> many(Parser<T> base) {
  return Container<ParseUnit<List<T>>>(new ManyParser<T>(base.p));
}
#endif

#ifndef NO_EOF_PACER
class EOFParser : public ParseUnit<bool> {
public:
  virtual PResult<bool> parse(Input inp, int pos) override {
    PREPARCE_CALLBACK
    if (inp.size() == pos)
      return {true, false, pos, ""};
    return {false, true, pos, "Expected end of file"};
  }
};

Parser<bool> eof() { return Container<ParseUnit<bool>>(new EOFParser()); }
#endif

#ifndef NO_INSIDE
template <class T, class TT, class TTT>
class InsideParser : public ParseUnit<T> {
  Parser<T> pmain;
  Parser<TT> pbeg;
  Parser<TTT> pend;

public:
  virtual PResult<T> parse(Input inp, int pos) override {
    PREPARCE_CALLBACK
    auto b = pbeg.parse(inp, pos);
    if (b.is_none) {
      PResult<T> out;
      out.is_none = b.is_none;
      out.err = b.err;
      return out;
    }

    auto out = pmain.parse(inp, b.pos);
    if (out.is_none) {
      return out;
    }

    auto e = pend.parse(inp, out.pos);
    out.pos = e.pos;
    out.err = e.err;
    out.is_none = e.is_none;
    return out;
  }

  InsideParser(Parser<TT> beg, Parser<T> main, Parser<TTT> end)
      : pbeg(beg), pmain(main), pend(end) {}
};

template <class T, class TT, class TTT>
Parser<T> inside(Parser<TT> beg, Parser<T> main, Parser<TTT> end) {
  return Container<ParseUnit<T>>(new InsideParser<T, TT, TTT>(beg, main, end));
}
#endif

#ifndef NO_SEPERATED
template <class T, class TT> class SeperatedParser : public ParseUnit<List<T>> {
  Container<ParseUnit<T>> main;
  Container<ParseUnit<TT>> sep;

public:
  SeperatedParser(Parser<T> main, Parser<TT> sep) : main(main.p), sep(sep.p) {}
  virtual PResult<List<T>> parse(Input inp, int pos) override {
    PREPARCE_CALLBACK
    List<T> out{};
    int npos = pos;
    do {
      auto o = main->parse(inp, npos);
      pos = npos;
      npos = o.pos;
      out.push_back(o.t);
      if (!o.is_none) {
        auto k = sep->parse(inp, npos);
        if (k.is_none)
          break;
        npos = k.pos;
      }
    } while (npos > pos);
    return PResult<List<T>>{out, false, npos};
  }
};

template <class T, class TT>
Parser<List<T>> seperated(Parser<T> main, Parser<TT> sep) {
  return Container<ParseUnit<List<T>>>(new SeperatedParser<T, TT>(main, sep));
}
#endif
}; // namespace parcer
