// Stand-ins for the BeipMU types MXP.cpp uses, so it can be compiled and exercised on Linux.
#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdint>
using uint8=std::uint8_t;

namespace Strings { struct Result { static constexpr int Not_Found=-1; }; }

static inline bool ieq(std::string_view a, std::string_view b){ if(a.size()!=b.size()) return false; for(size_t i=0;i<a.size();i++) if(tolower((unsigned char)a[i])!=tolower((unsigned char)b[i])) return false; return true; }

struct ConstString
{
   std::string_view v;
   constexpr ConstString() {}
   constexpr ConstString(const char *s) : v(s) {}
   constexpr ConstString(std::string_view s) : v(s) {}
   ConstString(const std::string &s) : v(s) {}
   explicit operator bool() const { return !v.empty(); }
   unsigned Count() const { return (unsigned)v.size(); }
   char operator[](unsigned i) const { return v[i]; }
   bool StartsWith(char c) const { return !v.empty() && v[0]==c; }
   bool StartsWith(ConstString s) const { return v.substr(0, s.v.size())==s.v; }
   bool EndsWith(char c) const { return !v.empty() && v.back()==c; }
   bool IStartsWith(ConstString s) const { return v.size()>=s.v.size() && ieq(v.substr(0,s.v.size()), s.v); }
   ConstString WithoutLast(unsigned n) const { return v.substr(0, v.size()-std::min<size_t>(n, v.size())); }
   ConstString First(unsigned n) const { return v.substr(0, std::min<size_t>(n, v.size())); }
   ConstString WithoutFirst(unsigned n) const { return v.substr(std::min<size_t>(n, v.size())); }
   int Find(ConstString s) const { auto p=v.find(s.v); return p==std::string_view::npos ? -1 : (int)p; }
   bool Split(char c, ConstString &l, ConstString &r) const { auto p=v.find(c); if(p==std::string_view::npos) return false; auto c2=v; l=c2.substr(0,p); r=c2.substr(p+1); return true; }
   bool Split(ConstString s, ConstString &l, ConstString &r) const { auto p=v.find(s.v); if(p==std::string_view::npos) return false; auto c2=v; l=c2.substr(0,p); r=c2.substr(p+s.v.size()); return true; }
   bool RSplit(char c, ConstString &l, ConstString &r) const { auto p=v.rfind(c); if(p==std::string_view::npos) return false; auto c2=v; l=c2.substr(0,p); r=c2.substr(p+1); return true; }
};
static inline bool IEquals(ConstString a, ConstString b){ return ieq(a.v,b.v); }

struct OwnedString
{
   std::string s;
   OwnedString() {}
   explicit OwnedString(ConstString c) : s(c.v) {}
   OwnedString &operator=(ConstString c) { s.assign(c.v); return *this; }
   explicit operator bool() const { return !s.empty(); }
   operator ConstString() const { return ConstString(std::string_view(s)); }
};

template<int N=0> struct HybridStringBuilder
{
   std::string s;
   HybridStringBuilder() {}
   template<typename... A> explicit HybridStringBuilder(A&&... a) { (*this)(a...); }
   void add(ConstString c) { s.append(c.v); }
   void add(const char *c) { s.append(c); }
   void add(char c) { s.push_back(c); }
   template<int M> void add(const HybridStringBuilder<M> &b) { s.append(b.s); }
   template<typename... A> HybridStringBuilder &operator()(A&&... a) { (add(a), ...); return *this; }
   operator ConstString() const { return ConstString(std::string_view(s)); }
};

template<typename T> struct Collection
{
   std::vector<T> v;
   void Push(T t) { v.push_back(std::move(t)); }
   void Pop() { v.pop_back(); }
   void Delete(unsigned i) { v.erase(v.begin()+i); }
   unsigned Count() const { return (unsigned)v.size(); }
   void Empty() { v.clear(); }
   T &operator[](unsigned i) { return v[i]; }
   auto begin() { return v.begin(); } auto end() { return v.end(); }
};

struct Color { int r{},g{},b{}; Color() {} Color(int r_,int g_,int b_):r(r_),g(g_),b(b_){} bool operator==(const Color&o) const { return r==o.r&&g==o.g&&b==o.b; } };

[[maybe_unused]] static int g_dpiScale=1;
#define g_szVersion "4.999"

template<typename T, typename... A> std::shared_ptr<T> MakeCounting(A&&... a) { return std::make_shared<T>(std::forward<A>(a)...); }
template<typename T, typename... A> std::unique_ptr<T> MakeUnique(A&&... a) { return std::make_unique<T>(std::forward<A>(a)...); }

struct Pueblo_Send { std::string m_send, m_hint; Pueblo_Send(ConstString s, ConstString h) : m_send(s.v), m_hint(h.v) {} };
struct CmdString : std::string { CmdString &operator=(ConstString c) { assign(c.v); return *this; } };
struct Command_Prompt { CmdString m_command; };

namespace Text
{
namespace Records
{
   struct Bold { bool v; operator bool() const { return v; } };
   struct Italic { bool v; operator bool() const { return v; } };
   struct Underline { bool v; operator bool() const { return v; } };
   struct Strikeout { bool v; operator bool() const { return v; } };
   struct Color { ::Color v; };
   struct ColorBg { ::Color v; };
   enum struct URLType { HTTP, Custom };
   struct URLData
   {
      std::string desc;
      URLData(Pueblo_Send &p) : desc("send:"+p.m_send+"|hint:"+p.m_hint) {}
      URLData(Command_Prompt &p) : desc("prompt:"+p.m_command) {}
      URLData(URLType, ConstString u) : desc("http:"+std::string(u.v)) {}
   };
}

struct LineBuilder
{
   Records::Bold b{}; Records::Italic i{}; Records::Underline u{}; Records::Strikeout s{}; Records::Color c{}; Records::ColorBg bg{};
   std::string url;
   std::string out; // Text with each change of style or link written as {..}
   template<typename T> T Get() const;
   void Set(Records::Bold x) { if(x.v!=b.v) out+=x.v?"{B}":"{/B}"; b=x; }
   void Set(Records::Italic x) { if(x.v!=i.v) out+=x.v?"{I}":"{/I}"; i=x; }
   void Set(Records::Underline x) { if(x.v!=u.v) out+=x.v?"{U}":"{/U}"; u=x; }
   void Set(Records::Strikeout x) { if(x.v!=s.v) out+=x.v?"{S}":"{/S}"; s=x; }
   void Set(Records::Color x) { if(!(x.v==c.v)) { char t[32]; snprintf(t,32,"{fg %d,%d,%d}",x.v.r,x.v.g,x.v.b); out+=t; } c=x; }
   void Set(Records::ColorBg x) { if(!(x.v==bg.v)) { char t[32]; snprintf(t,32,"{bg %d,%d,%d}",x.v.r,x.v.g,x.v.b); out+=t; } bg=x; }
   void SetURL(std::unique_ptr<Records::URLData> p) { std::string n=p?p->desc:""; if(n!=url) out+= p ? "{link "+n+"}" : std::string("{/link}"); url=n; }
   void AppendChar(char ch) { out.push_back(ch); }
   void AppendString(ConstString s2) { out.append(s2.v); }
   void AppendEmoji(const char *e) { out.append(e); }
   void AppendNewLine() { out+="{BR}"; }
};
template<> inline Records::Bold LineBuilder::Get<Records::Bold>() const { return b; }
template<> inline Records::Italic LineBuilder::Get<Records::Italic>() const { return i; }
template<> inline Records::Underline LineBuilder::Get<Records::Underline>() const { return u; }
template<> inline Records::Strikeout LineBuilder::Get<Records::Strikeout>() const { return s; }
template<> inline Records::Color LineBuilder::Get<Records::Color>() const { return c; }
template<> inline Records::ColorBg LineBuilder::Get<Records::ColorBg>() const { return bg; }
}

namespace Streams
{
struct Input
{
   ConstString text; unsigned pos{};
   Input(ConstString t) : text(t) {}
   unsigned char CharGet() { return pos<text.Count() ? text[pos++] : 0; }
   unsigned char CharSpy() const { return pos<text.Count() ? text[pos] : 0; }
   bool CharSkip(char c) { if(CharSpy()==(unsigned char)c) { pos++; return true; } return false; }
   void CharBack() { pos--; }
   bool Parse(unsigned &n) { unsigned p=pos; n=0; while(isdigit(CharSpy())) n=n*10+(CharGet()-'0'); return pos!=p; }
   unsigned PosGet() const { return pos; }
   ConstString GetText() const { return text; }
   bool SkipUntilChar(char c) { while(unsigned char x=CharGet()) if(x==(unsigned char)c) return true; return false; }
   struct PosRestorer { Input &s; unsigned p; bool keep{}; PosRestorer(Input &i) : s(i), p(i.pos) {} ~PosRestorer() { if(!keep) s.pos=p; } void NoRestore() { keep=true; } };
};
}

namespace Text
{
// A simple HTML attribute reader in the shape of BeipMU's
struct HTMLParser
{
   struct Attribute { ConstString m_name, m_value; };
   Streams::Input &s;
   HTMLParser(Streams::Input &i, LineBuilder &) : s(i) {}
   Streams::Input &GetStream() { return s; }
   void SkipSpace() { while(s.CharSpy()==' ') s.CharGet(); }
   bool ParseName(ConstString &name) { unsigned p=s.pos; while(isalnum(s.CharSpy()) || s.CharSpy()=='_' || s.CharSpy()=='.') s.CharGet(); if(p==s.pos) return false; name=s.text.v.substr(p, s.pos-p); return true; }
   bool ParseValue(ConstString &value)
   {
      Streams::Input::PosRestorer r(s); SkipSpace();
      unsigned char q=s.CharSpy();
      if(q=='"' || q=='\'') { s.CharGet(); unsigned p=s.pos; while(s.CharSpy() && s.CharSpy()!=q) s.CharGet(); if(!s.CharSpy()) return false; value=s.text.v.substr(p, s.pos-p); s.CharGet(); r.NoRestore(); return true; }
      unsigned p=s.pos; while(s.CharSpy() && s.CharSpy()!=' ' && s.CharSpy()!='>' && !(s.CharSpy()=='/' && s.text.v.substr(s.pos,2)=="/>")) s.CharGet();
      if(p==s.pos)
         return false;
      value=s.text.v.substr(p, s.pos-p); r.NoRestore(); return true;
   }
   bool ParseAttribute(Attribute &a)
   {
      Streams::Input::PosRestorer r(s); SkipSpace();
      ConstString name; if(!ParseName(name)) return false;
      if(!s.CharSkip('=')) return false;
      ConstString value; if(!ParseValue(value)) return false;
      a.m_name=name; a.m_value=value; r.NoRestore(); return true;
   }
   ConstString ParseEntity() { return {}; }
};
}
