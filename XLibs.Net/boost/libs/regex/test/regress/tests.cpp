/*
 *
 * Copyright (c) 1998-2002
 * Dr John Maddock
 *
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *
  *   FILE     tests.cpp
  *   VERSION  see <boost/version.hpp>
  *
  * the actual tests conducted by regress.
  *
  */

#include <cassert>
#include <boost/regex.hpp>
#include "regress.h"

#ifndef BOOST_REGEX_NO_TEST

# ifdef BOOST_MSVC
#  pragma warning(disable: 4244 4267)
#endif

using namespace boost;

template <class M1, class M2>
bool compare_result(const M1& sm, const M2& m)
{
   if(sm.size() != m.size())
      return false;
#ifdef BOOST_REGEX_V3
   if(sm.line() != m.line())
      return false;
#endif
   for(unsigned int i = 0; i < sm.size(); ++i)
   {
      if(sm.position(i) != m.position(i))
         return false;
      if(sm.length(i) != m.length(i))
         return false;
   }
   return true;
}

template <class M1>
bool compare_result(const M1& sm, const M1& m)
{
   return sm == m;
}


template <class C, class T, class A>
void cpp_eh_tests(const basic_regex<C, T, A>& )
{
#ifndef __GNUC__
   bool thrown = false;
   // try set_expression form first:
#ifndef BOOST_NO_EXCEPTIONS
   try
   {
#endif
      A a;
      basic_regex<C, T, A> e(a);
      e.set_expression(expression.c_str(), flags[2] | regex::use_except);
#ifndef BOOST_NO_EXCEPTIONS
   }
   catch(const boost::bad_expression&)
   {
      thrown = true;
   }
   catch(...){}
   if(!thrown)
   {
      begin_error();
      cout << "Error: expected exception not thrown" << endl;
   }
#endif

   // now try constructor form:
   thrown = false;
#ifndef BOOST_NO_EXCEPTIONS
   try
#endif
   {
      A a;
      basic_regex<C, T, A> e(expression.c_str(), flags[2] | regex::use_except, a);
   }
#ifndef BOOST_NO_EXCEPTIONS
   catch(const boost::bad_expression&)
   {
      thrown = true;
   }
   catch(...){}
   if(!thrown)
   {
      begin_error();
      cout << "Error: expected exception not thrown" << endl;
   }
#endif
#endif
}

template <class iterator>
iterator find_last_line(iterator start, iterator end)
{
   iterator result = start;
   while(start != end)
   {
      if(*start == '\n')
      {
         ++start;
         result = start;
      }
      else
         ++start;
   }
   return result;
}

template <class iterator>
unsigned int count_lines(iterator start, iterator end)
{
   unsigned int result = 0;
   while(start != end)
   {
      if(*start == '\n')
         ++result;
      ++start;
   }
   return result;
}

template <class iterator, class Alloc>
class grep_test_predicate
{
   int match_id;
   iterator base, term;
   // rebind allocator to correct type:
#ifdef BOOST_REGEX_V3
   typedef Alloc alloc_type;
#else
   typedef typename detail::rebind_allocator<boost::sub_match<iterator>, Alloc>::type alloc_type;
#endif
public:
   grep_test_predicate(iterator i, iterator j) : base(i), term(j) { match_id = 0; }
   ~grep_test_predicate(){}
   bool operator()(const boost::match_results< iterator, alloc_type >& m);
};

template <class iterator, class Alloc>
bool grep_test_predicate<iterator, Alloc>::operator()(const boost::match_results< iterator, alloc_type >& m)
{
   std::ptrdiff_t start, end;
   start = m[0].first - base;
   end = m[0].second - base;
   if((matches[match_id] != start) || (matches[match_id + 1] != end))
   {
      begin_error();
      cout << "regex++ grep match error: found [" << start << "," << end << "] expected [" << matches[match_id] << "," << matches[match_id+1] << "]" << endl;
   }

   //
   // check $`:
   start = m[-1].first - base;
   end = m[-1].second - base;
   if(match_id &&
      ( (end != matches[match_id]) || (start != matches[match_id - 1]) )
   )
   {
      begin_error();
      cout << "regex++ grep error in $`: found [" << start << "," << end << "] expected [" << matches[match_id-1] << "," << matches[match_id] << "]" << endl;
   }
   else if((!match_id) && ((start != 0) || (end != matches[0])))
   {
      begin_error();
      cout << "regex++ grep error in $`: found [" << start << "," << end << "] expected [" << 0 << "," << matches[0] << "]" << endl;
   }

   //
   // check $':
   start = m[-2].first - base;
   end = m[-2].second - base;
   if((start != matches[match_id + 1]) || (end != (term-base)))
   {
      begin_error();
      cout << "regex++ grep error in $': found [" << start << "," << end << "] expected [" << matches[match_id + 1] << "," << (term-base) << "]" << endl;
   }

   //
   // now check line()
   /* don't check this, it's not supported in the new algorithm....
   start = m.line();
   end = count_lines(base, iterator(m[0].first)) + 1;
   if(start != end)
   {
      begin_error();
      cout << "regex++ grep error in line(): found " << start << " expected " << end << endl;
   }
   */
   //
   // now check line_start()
   /* don't check this, it's not supported in the new algorithm....
   start = m.line_start() - base;
   end = find_last_line(base, iterator(m[0].first)) - base;
   if(start != end)
   {
      begin_error();
      cout << "regex++ grep error in line_start(): found " << start << " expected " << end << endl;
   }
   */
   match_id += 2;
   return true;
}

template <class C, class T, class A>
void cpp_tests(const basic_regex<C, T, A>& e, bool recurse = true)
{
   typedef typename basic_regex<C, T, A>::allocator_type base_allocator_type;
#ifdef BOOST_REGEX_V3
   typedef base_allocator_type allocator_type;
#else
   typedef typename detail::rebind_allocator<boost::sub_match<debug_iterator<string_type::iterator> >, base_allocator_type>::type allocator_type;
#endif
   if(flags[4] & REG_MERGE)
   {
      //
      // test merge code:
      //
      string_type s;
      s = regex_merge(search_text, e, format_string.c_str(), static_cast<boost::match_flag_type>(flags[3]));
      if(s != merge_string)
      {
         begin_error();
         cout << "merge result mismatch: found \"" << make_narrow(s).c_str() << "\" expected \"" << make_narrow(merge_string.c_str()) << "\"" << endl;
      }
      return;
   }


   if(recurse)
   {
      // copy and assign test:
      basic_regex<C, T, A> e2(e);
      cpp_tests(e2, false);
      e2 = e;
      cpp_tests(e2, false);
      basic_regex<C, T, A> e3;
#ifndef BOOST_REGEX_V3
      e3.swap(e2);
      cpp_tests(e3, false);
#endif
   }

   if(e.error_code())
   {
      // verify that we don't expect expression to compile
      if(search_text != BOOST_RE_STR("!"))
      {
         begin_error();
         cout << "Expression did not compile using regex++ API" << endl;
      }
      else if((recurse) && ((flags[3] & match_partial) == 0))
         cpp_eh_tests(e);
   }
   else if(flags[4] & REG_GREP)
   {
      // try to do grep:
      debug_iterator<string_type::iterator> x(search_text.begin(), search_text.begin(), search_text.end());
      debug_iterator<string_type::iterator> y(search_text.end(), search_text.begin(), search_text.end());
      grep_test_predicate<debug_iterator<string_type::iterator>, allocator_type> oi(x, y);
      regex_grep(oi, x, y, e, static_cast<boost::match_flag_type>(flags[3]));
#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
      if(!recurse)
      {
         unsigned len = search_text.size();
         const std::basic_string<char_t>& s = search_text;
         grep_test_predicate<std::basic_string<char_t>::const_iterator, base_allocator_type> oi2(s.begin(), s.end());
         regex_grep(oi2, s, e, static_cast<boost::match_flag_type>(flags[3]));
         grep_test_predicate<const char_t*, base_allocator_type> oi3(s.c_str(), s.c_str()+s.size());
         regex_grep(oi3, s.c_str(), e, static_cast<boost::match_flag_type>(flags[3]));
         assert(s.size() == len);
         assert(s.end() - s.begin() == (std::basic_string<char_t>::difference_type)len);
      }
#endif
   }
   else
   {
      // try to find match
      match_results< debug_iterator<string_type::iterator>, allocator_type> m;
      debug_iterator<string_type::iterator> x(search_text.begin(), search_text.begin(), search_text.end());
      debug_iterator<string_type::iterator> y(search_text.end(), search_text.begin(), search_text.end());
      if(regex_search(x, y, m, e, static_cast<boost::match_flag_type>(flags[3])))
      {
         // special case for partial matches:
         if(flags[4] & REG_PARTIAL_MATCH)
         {
            if(m[0].matched)
            {
               begin_error();
               cout << "regex++ API result mismatch, found full match when partial match was expected: found ("
                        << (m[0].first - x) << "," <<
                        (m[0].second - x) << ")" << endl;
            }
            else if(((m[0].first - x) != matches[0]) || ((m[0].second - x) != matches[1]))
            {
               begin_error();
               cout << "regex++ API result mismatch in sub-expression " << 0 <<
                       ", found (" << (m[0].first - x) << "," <<
                       (m[0].second - x) << ") expected (" <<
                       matches[0] << "," << matches[1] << ")" << endl;
            }
            return; // don't bother testing anything else for partial matches
         }
         // match found compare what matched with what we expect:
         int j = 0;
         for(unsigned int i = 0; i < m.size(); ++i, j += 2)
         {
            if(m[i].matched == false)
            {
               if(matches[j] != -1)
               {
                  begin_error();
                  cout << "regex++ API result mismatch in sub-expression " << i <<
                          ", found (" << (m[i].first - x) << "," <<
                          (m[i].second - x) << ") expected (" <<
                          matches[j] << "," << matches[j+1] << ")" << endl;
               }
            }
            else if(((m[i].first - x) != matches[j]) || ((m[i].second - x) != matches[j+1]))
            {
               begin_error();
               cout << "regex++ API result mismatch in sub-expression " << i <<
                       ", found (" << (m[i].first - x) << "," <<
                       (m[i].second - x) << ") expected (" <<
                       matches[j] << "," << matches[j+1] << ")" << endl;
            }
         }
         //
         // now check $` and $':
         //
         if((m[-1].first != x) || (m[-1].second != m[0].first))
         {
            begin_error();
            cout << "regex++ API result mismatch in $` (match -1), found (" <<
               (m[-1].first - x) << "," << (m[-1].second - x) << ") expected (0" <<
               "," << matches[0] << ")" << endl;
         }
         if(((flags[3] & match_partial) == 0) && ((m[-2].first != m[0].second) || (m[-2].second != y)))
         {
            begin_error();
            cout << "regex++ API result mismatch in $' (match -2), found (" <<
               (m[-2].first - x) << "," << (m[-2].second - x) << ") expected (" <<
               matches[1] << "," << (y-x) << ")" << endl;
         }
#if !(defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)) && !defined(BOOST_REGEX_V3)
         //
         // now try comparison operators:
         string_type s(m[0]);
         if((s != m[0]) || (m[0] != s)
            || !(s == m[0]) || !(m[0] == s)
            || (s < m[0]) || (m[0] < s)
            || (s > m[0]) || (m[0] > s)
            || !(s <= m[0]) || !(m[0] <= s)
            || !(s >= m[0]) || !(m[0] >= s))
         {
            begin_error();
            cout << "string comparison failed for result" << std::endl;
         }
         if(s.find_first_of((string_type::value_type)0) == string_type::npos)
         {
            if((m[0] != s.c_str()) || (s.c_str() != m[0])
               || !(m[0] == s.c_str()) || !(s.c_str() == m[0])
               || (m[0] > s.c_str()) || (s.c_str() > m[0])
               || (m[0] < s.c_str()) || (s.c_str() < m[0])
               || !(m[0] >= s.c_str()) || !(s.c_str() >= m[0])
               || !(m[0] <= s.c_str()) || !(s.c_str() <= m[0]))
            {
               begin_error();
               cout << "string comparison failed for result" << std::endl;
            }
         }
         //
         // now try addition operators:
         string_type sa(1, (string_type::value_type)'a');
         if(
            ((m[0] + sa) != (s + sa))
            || (sa + m[0]) != (sa + s)
            || ((m[0] + m[0]) != (s + s))
            || ((m[0] + m[0]) != (s + s))
            || ((m[0] + sa[0]) != (s + sa[0]))
            || ((sa[0] + m[0]) != (sa[0] + s))
            )
         {
               begin_error();
               cout << "string addition failed for result" << std::endl;
         }
         if(s.find_first_of((string_type::value_type)0) == string_type::npos)
         {
            if(
               ((m[0] + sa.c_str()) != (s + sa))
               || ((sa.c_str() + m[0]) != (sa + s))
              )
            {
                  begin_error();
                  cout << "string addition failed for result" << std::endl;
            }
         }
#endif

         //
         // now try alternative forms of regex_search if available:
#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
         if(!recurse)
         {
            std::basic_string<char_t> s2(search_text.begin(), search_text.end());
            match_results<std::basic_string<char_t>::const_iterator> sm;
            if(regex_search(s2, sm, e, static_cast<boost::match_flag_type>(flags[3])))
            {
               if(compare_result(sm, m) == false)
               {
                  begin_error();
                  cout << "regex++ API result mismatch in regex_search(const std::string&, match_results&, const basic_regex&, int)" << endl;
               }
            }
            else
            {
               begin_error();
               cout << "regex++ API result mismatch in regex_search(const std::string&, match_results&, const basic_regex&, int)" << endl;
            }
#ifndef BOOST_REGEX_V3
            if(!regex_search(s2, e, static_cast<boost::match_flag_type>(flags[3]))
               || !regex_search(s2.begin(), s2.end(), e, static_cast<boost::match_flag_type>(flags[3])))
            {
               begin_error();
               cout << "regex++ API result mismatch in regex_search(const std::string&, const basic_regex&, int)" << endl;
            }
#endif
            //
            // partial match should give same result as full match
            // provided a full match is expected:
            //
            if(matches[0] > 0)
            {
               if(regex_search(x, y, m, e, static_cast<boost::match_flag_type>(flags[3]) | boost::match_partial))
               {
                  if(compare_result(sm, m) == false)
                  {
                     begin_error();
                     cout << "regex++ API result mismatch in regex_search when enabling match_partial" << endl;
                  }
               }
               else
               {
                  begin_error();
                  cout << "regex++ API result: match not found when match_partial specified" << endl;
               }
            }
            if(s2.find(char_t(0)) == std::basic_string<char_t>::npos)
            {
               match_results<const char_t*> ssm;
               if(regex_search(search_text.c_str(), ssm, e, static_cast<boost::match_flag_type>(flags[3])))
               {
                  if(compare_result(ssm, m) == false)
                  {
                     begin_error();
                     cout << "regex++ API result mismatch in regex_search(const char_t*, match_results&, const basic_regex&, int)" << endl;
                  }
               }
               else
               {
                  begin_error();
                  cout << "regex++ API result mismatch in regex_search(const char_t*, match_results&, const basic_regex&, int)" << endl;
               }
#ifndef BOOST_REGEX_V3
               if(!regex_search(search_text.c_str(), e, static_cast<boost::match_flag_type>(flags[3])))
               {
                  begin_error();
                  cout << "regex++ API result mismatch in regex_search(const char_t*, const basic_regex&, int)" << endl;
               }
#endif
            }
         }
         if((false == recurse) && (matches[0] == 0) && (matches[1] == static_cast<int>(search_text.size())))
         {
            //
            // match expected on whole string, so all versions
            // of regex_match should also succeed:
            //
            match_results< debug_iterator<string_type::iterator>, allocator_type> m1;
            debug_iterator<string_type::iterator> x1(search_text.begin(), search_text.begin(), search_text.end());
            debug_iterator<string_type::iterator> y1(search_text.end(), search_text.begin(), search_text.end());
            if(regex_match(x1, y1, m1, e, static_cast<boost::match_flag_type>(flags[3])))
            {
               if(compare_result(m1, m) == false)
               {
                  begin_error();
                  cout << "regex++ API result mismatch in regex_match(iterator, iterator, match_results&, const basic_regex&, int)" << endl;
               }
            }
            else
            {
               begin_error();
               cout << "regex++ API result mismatch in regex_match(iterator, iterator, match_results&, const basic_regex&, int)" << endl;
            }
            std::basic_string<char_t> s3(search_text.begin(), search_text.end());
            match_results<std::basic_string<char_t>::const_iterator> sm;
            if(regex_match(s3, sm, e, static_cast<boost::match_flag_type>(flags[3])))
            {
               if(compare_result(sm, m) == false)
               {
                  begin_error();
                  cout << "regex++ API result mismatch in regex_match(const std::string&, match_results&, const basic_regex&, int)" << endl;
               }
            }
            else
            {
               begin_error();
               cout << "regex++ API result mismatch in regex_match(const std::string&, match_results&, const basic_regex&, int)" << endl;
            }
            if(s3.find(char_t(0)) == std::basic_string<char_t>::npos)
            {
               match_results<const char_t*> ssm;
               if(regex_match(search_text.c_str(), ssm, e, static_cast<boost::match_flag_type>(flags[3])))
               {
                  if(compare_result(ssm, m) == false)
                  {
                     begin_error();
                     cout << "regex++ API result mismatch in regex_match(const char_t*, match_results&, const basic_regex&, int)" << endl;
                  }
               }
               else
               {
                  begin_error();
                  cout << "regex++ API result mismatch in regex_match(const char_t*, match_results&, const basic_regex&, int)" << endl;
               }
            }
         }
#endif
      }
      else
      {
         // match not found
         if(matches[0] != -1)
         {
            begin_error();
            cout << "Match expected but not found using regex++ API" << endl;
         }
      }
   }
}

#if !defined(TEST_UNICODE)

unsigned int hl_match_id;

bool
#if (defined(__BORLANDC__) || defined(BOOST_MSVC)) && !defined(BOOST_DISABLE_WIN32)
__cdecl
#endif
hl_grep_test_proc(const RegEx& e)
{
   std::ptrdiff_t start, end;

   start = e.Position(0);
   end = start + e.Length();
   if((matches[hl_match_id] != start) || (matches[hl_match_id + 1] != end))
   {
      begin_error();
      cout << "class RegEx grep match error: found [" << start << "," << end << "] expected [" << matches[hl_match_id] << "," << matches[hl_match_id+1] << "]" << endl;
   }
   if(0 == (flags[4] & REG_GREP))
   {
      for(unsigned int sub = 1; sub < e.Marks(); ++sub)
      {
         start = e.Position(sub);
         end = start + e.Length(sub);
         if((matches[2*sub] != start) || (matches[2*sub + 1] != end))
         {
            begin_error();
            cout << "class RegEx grep match error: found in sub " << sub << " [" << start << "," << end << "] expected [" << matches[2*sub] << "," << matches[2*sub+1] << "]" << endl;
         }
      }
   }

   //
   // check $`:
   start = e.Position(-1);
   end = start + e.Length(-1);
   if(start == -1)
   {
      if(hl_match_id &&
         ( matches[hl_match_id] != matches[hl_match_id - 1] )
      )
      {
         begin_error();
         cout << "class RegEx grep error in $`: found [" << start << "," << end << "] expected [" << matches[hl_match_id-1] << "," << matches[hl_match_id] << "]" << endl;
      }
      else if((!hl_match_id) && (0 != matches[0]))
      {
         begin_error();
         cout << "class RegEx grep error in $`: found [" << start << "," << end << "] expected [" << 0 << "," << matches[0] << "]" << endl;
      }
   }
   else
   {
      if(hl_match_id &&
         ( (end != matches[hl_match_id]) || (start != matches[hl_match_id - 1]) )
      )
      {
         begin_error();
         cout << "class RegEx grep error in $`: found [" << start << "," << end << "] expected [" << matches[hl_match_id-1] << "," << matches[hl_match_id] << "]" << endl;
      }
      else if((!hl_match_id) && ((start != 0) || (end != matches[0])))
      {
         begin_error();
         cout << "class RegEx grep error in $`: found [" << start << "," << end << "] expected [" << 0 << "," << matches[0] << "]" << endl;
      }
   }

   //
   // check $':
   start = e.Position(-2);
   end = start + e.Length(-2);
   if(start == -1)
   {
      if(matches[hl_match_id + 1] != (int)search_text.size())
      {
         begin_error();
         cout << "class RegEx grep error in $': found [" << start << "," << end << "] expected [" << matches[hl_match_id + 1] << "," << (search_text.size()) << "]" << endl;
      }
   }
   else if((start != matches[hl_match_id + 1]) || (end != (int)search_text.size()))
   {
      begin_error();
      cout << "class RegEx grep error in $': found [" << start << "," << end << "] expected [" << matches[hl_match_id + 1] << "," << (search_text.size()) << "]" << endl;
   }

   hl_match_id += 2;
   return true;
}

void cpp_hl_tests(RegEx& e, bool recurse = true)
{
   if(flags[4] & REG_MERGE)
      return;

   if(e.error_code())
   {
      if(search_text != BOOST_RE_STR("!"))
      {
         begin_error();
         cout << "Expression did not compile with class RegEx" << endl;
      }
      return;
   }

   if(recurse)
   {
      // copy and assign test:
      RegEx e2(e);
      cpp_hl_tests(e2, false);
      e2 = e;
      cpp_hl_tests(e2, false);
   }

   if(flags[4] & REG_GREP)
   {
      // try to do grep:
      hl_match_id = 0;
      GrepCallback cb = hl_grep_test_proc;
      e.Grep(cb, search_text.c_str(), static_cast<boost::match_flag_type>(flags[3]));
   }
   else
   {
      if(e.Search(search_text.c_str(), static_cast<boost::match_flag_type>(flags[3])))
      {
         unsigned int i = 0;
         unsigned int j = 0;
         while(matches[j] != -2)
         {
            if( (matches[j] != (int)e.Position(i))
               || ((matches[j] != -1) && ((matches[j+1] - matches[j] != (int)e.Length(i)))) )
            {
               begin_error();
               cout << "RegEx::Search error in subexpression " << i << ": found [" << e.Position(i) << "," << (e.Position(i) + e.Length(i)) << "] expected [" << matches[j] << "," << matches[j+1] << "]" << endl;
            }
            ++i;
            j += 2;
         }
      }
      else
      {
         if(matches[0] != -1)
         {
            begin_error();
            cout << "match expected but not found with RexEx::Search" << endl;
         }
      }

      //
      // test RegEx::Match only if we expect to match all of the input:
      //
      if((matches[0] == 0) && (matches[1] == (int)search_text.size()))
      {
         if(e.Match(search_text.c_str(), static_cast<boost::match_flag_type>(flags[3])))
         {
            unsigned int i = 0;
            unsigned int j = 0;
            while(matches[j] != -2)
            {
               if( (matches[j] != (int)e.Position(i))
                  || ((matches[j] != -1) && ((matches[j+1] - matches[j] != (int)e.Length(i)))) )
               {
                  begin_error();
                  cout << "RegEx::Match error in subexpression " << i << ": found [" << e.Position(i) << "," << (e.Position(i) + e.Length(i)) << "] expected [" << matches[j] << "," << matches[j+1] << "]" << endl;
               }
               ++i;
               j += 2;
            }
         }
         else
         {
            begin_error();
            cout << "Match expected but not found with RegEx::Match" << endl;
         }
      }
   }
}
#endif

template <class iterator>
bool has_nulls(iterator i, iterator j)
{
   while(i != j)
   {
      if(*i == 0)
         return true;
      ++i;
   }
   return false;
}

#ifdef TEST_UNICODE
#undef regcomp
#undef regerror
#undef regexec
#undef regfree
#undef regex_t
#define regcomp regcompW
#define regerror regerrorW
#define regexec regexecW
#define regfree regfreeW
#define regex_t regex_tW
#endif


void run_tests()
{
#ifndef TEST_UNICODE
   if(flags[4] & REG_UNICODE_ONLY)
      return;
#endif
#ifndef NO_CPP_TEST
#ifndef BOOST_NO_EXCEPTIONS
   try
   {
#endif
      unsigned int f = flags[2] & ~regex::use_except;
      if(flags[0] & REG_ICASE)
         f |= regex::icase;
      re_type e(expression.c_str(), f);
      cpp_tests(e, true);
#ifndef BOOST_NO_EXCEPTIONS
   }
   catch(const std::exception& e)
   {
      //
      // this is only an error if the expression is expected to be valid:
      if(search_text != BOOST_RE_STR("!"))
      {
         begin_error();
         cout << "Unexpected exception thrown from C++ library: " << e.what() << endl;
      }
   }
   catch(...)
   {
      begin_error();
      cout << "Unexpected exception thrown from C++ library" << endl;
   }
#endif // BOOST_NO_EXCEPTIONS
#endif

#if !defined(TEST_UNICODE)
#ifndef BOOST_NO_EXCEPTIONS
   try
   {
#endif
      if(((flags[3] & match_partial) == 0) && (flags[2] == regex::normal) && (has_nulls(search_text.begin(), search_text.end()) == false))
      {
         RegEx e;
         e.SetExpression(expression.c_str(), flags[0] & REG_ICASE);
         cpp_hl_tests(e, true);
      }
#ifndef BOOST_NO_EXCEPTIONS
   }
   catch(const std::exception& )
   {
      if(search_text != BOOST_RE_STR("!"))
      {
         begin_error();
         cout << "Expression did not compile with class RegEx" << endl;
      }
   }
   catch(...)
   {
      begin_error();
      cout << "Unexpected exception thrown from RegEx::SetExpression" << endl;
   }
#endif // BOOST_NO_EXCEPTIONS
#endif

   if(flags[4] & (REG_NO_POSIX_TEST | REG_GREP | REG_MERGE | REG_MERGE_COPY))
      return;
   regex_t posix_expression;
   regmatch_t m[MAX_MATCHES];
   if(regcomp(&posix_expression, expression.c_str(), flags[0]) == 0)
   {
      if(flags[1] & REG_STARTEND)
      {
         m[0].rm_so = 0;
         m[0].rm_eo = search_text.size();
      }
      if(regexec(&posix_expression, search_text.c_str(), MAX_MATCHES, m, flags[1]))
      {
         // match not found
         if(matches[0] != -1)
         {
            begin_error();
            cout << "Match expected but not found using POSIX API" << endl;
         }
      }
      else
      {
         // match found compare what matched with what we expect:
         int j = 0;
         for(unsigned int i = 0; i <= posix_expression.re_nsub; ++i, j += 2)
         {
            if((m[i].rm_so != matches[j]) || (m[i].rm_eo != matches[j+1]))
            {
               begin_error();
               cout << "POSIX API result mismatch in sub-expression " << i << ", found (" << m[i].rm_so << "," << m[i].rm_eo <<
                        ") expected (" << matches[j] << "," << matches[j+1] << ")" << endl;
            }
         }
      }
      regfree(&posix_expression);
   }
   else
   {
      // expression did not compile
      if(search_text != BOOST_RE_STR("!"))
      {
         begin_error();
         cout << "Expression did not compile using POSIX API" << endl;
      }
   }
}



//
// error reporting:
int last_line = 0;

void begin_error()
{
   if(line != last_line)
   {
      cout << "Error in line " << line << " of file " << file << endl;
      cout << "Expression: " << make_narrow(expression).c_str() << endl;
      cout << "Search text: " << make_narrow(search_text).c_str() << endl;
      cout << "Flags: ";
      bool started = false;
      unsigned int id = 0;
      while(flag_data[id].len != 0)
      {
         if(flags[flag_data[id].id] & flag_data[id].value)
         {
            if(started)
               cout << " | ";
            cout << make_narrow(flag_data[id].name).c_str();
            started = true;
         }
         ++id;
      }
      cout << endl;
      last_line = line;
      ++error_count;
   }
}

void reset_error()
{
   last_line = 0;
}

#if defined(__BORLANDC__) && (__BORLANDC__ >= 0x550) && (__BORLANDC__ <= 0x551) && !defined(_RWSTD_COMPILE_INSTANTIATE)
//
// this is an ugly hack to work around an ugly problem:
// by default this file will produce unresolved externals during
// linking unless _RWSTD_COMPILE_INSTANTIATE is defined (Borland bug).
// However if _RWSTD_COMPILE_INSTANTIATE is defined then we get separate
// copies of basic_string's static data in the RTL and this DLL, this messes
// with basic_string's memory management and results in run-time crashes,
// Oh sweet joy of Catch 22....
//
namespace std{
template<> template<>
basic_string<string_type::value_type>&
basic_string<string_type::value_type>
   ::replace<debug_iterator<string_type::iterator> >(
         string_type::iterator f1, 
         string_type::iterator f2, 
         debug_iterator<string_type::iterator> i1, 
         debug_iterator<string_type::iterator> i2)
{
   unsigned insert_pos = f1 - begin();
   unsigned remove_len = f2 - f1;
   unsigned insert_len = i2 - i1;
   unsigned org_size = size();
   if(insert_len > remove_len)
   {
      append(insert_len-remove_len, ' ');
      std::copy_backward(begin() + insert_pos + remove_len, begin() + org_size, end());
      std::copy(i1, i2, begin() + insert_pos);
   }
   else
   {
      std::copy(begin() + insert_pos + remove_len, begin() + org_size, begin() + insert_pos + insert_len);
      std::copy(i1, i2, begin() + insert_pos);
      erase(size() + insert_len - remove_len);
   }
   return *this;
}
}
#endif



#endif






