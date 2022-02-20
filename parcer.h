#pragma ocne
#include <string>
#include <memory>
#include <vector>

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
    PResult<T> parse(Input inp, int pos) { return p->parse(inp, pos); }
  };

  template <class TT, class T> class ParserPipe : public ParseUnit<T> {
    Container<ParseUnit<TT>> p1;
    Container<ParseUnit<T>> p2;

  public:
    virtual PResult<T> parse(Input inp, int pos) override {
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

  template <class TT, class T>
  Parser<T> operator>>(Parser<TT> p1, Parser<T> p2) {
    return Container<ParseUnit<T>>(new ParserPipe<TT, T>(p1.p, p2.p));
  }

  template <class TT, class T> class RevParserPipe : public ParseUnit<TT> {
    Container<ParseUnit<TT>> p1;
    Container<ParseUnit<T>> p2;

  public:
    virtual PResult<TT> parse(Input inp, int pos) override {
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
      auto r1 = p1->parse(inp, pos);
      if (r1.is_none)
        return p2->parse(inp, pos);
      return r1;
    }
    ParserOr(Container<ParseUnit<T>> p1, Container<ParseUnit<T>> p2)
        : p1(p1), p2(p2) {}
  };

  template <class T> Parser<T> operator||(Parser<T> p1, Parser<T> p2) {
    return Container<ParseUnit<T>>(new ParserOr<T>(p1, p2));
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

  template <class T> static Parser<List<T>> many(Parser<T> base) {
    return Container<ParseUnit<std::vector<T>>>(new ManyParser<T>(base.p));
  }
#endif

#ifndef no_EOF_PACER
  class EOFParser : public ParseUnit<bool> {
  public:
    virtual PResult<bool> parse(Input inp, int pos) override {
      if (inp.size() == pos)
        return {true, false, pos, ""};
      return {false, true, pos, "Expected end of file"};
    }
  };

  Parser<bool> eof() { return Container<ParseUnit<bool>>(new EOFParser()); }
#endif
};
